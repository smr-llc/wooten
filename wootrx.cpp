#include <Bela.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include "config.h"
#include "wootrx.h"
#include "wootpkt.h"


WootRx::WootRx() :
	m_sock(0),
	m_readPos(0),
	m_writePos(0),
	m_rxQueueSize(8)
{
	m_resampler = resamp2_crcf_create(4, 0.0f, 60.0f);
}

WootRx::~WootRx() {
	resamp2_crcf_destroy(m_resampler);
}

int WootRx::init(in_port_t port) {
	m_peerSockLen = sizeof(m_peerAddr);

	if ((m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		printf("FATAL: Failed to create udp receive socket, got errno %d\n", errno);
		fflush(stdout);
		return 1;
	}

	struct sockaddr_in addr;
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if( bind(m_sock, (struct sockaddr*)&addr, sizeof(addr) ) == -1)
	{
		printf("FATAL: Failed to bind udp receive socket, got errno %d\n", errno);
		fflush(stdout);
		return 2;
	}

	return 0;
}

LevelMeter * const WootRx::levelMeter() {
	return &m_meter;
}

void WootRx::setRxQueueSize(int size) {
	if (size < 1 || size > MAX_RX_QUEUE_SIZE) {
		return;
	}
	m_rxQueueSize = size;
}

int WootRx::rxQueueSize() const {
	return m_rxQueueSize;
}

void WootRx::rxUdp(void* rxArg) {
	WootRx* rx = (WootRx*)rxArg;

	printf("Starting rxUdp...\n");
	fflush(stdout);

	printf("Payload size: %d\n", sizeof(WootPkt));

    WootPkt pkt;
	int lastSeq = -1;

	while (!Bela_stopRequested()) {

		while (true) {
			int nBytes = recvfrom(rx->m_sock,
									(char*)&pkt,
									sizeof(WootPkt),
									MSG_DONTWAIT,
									(struct sockaddr *) &rx->m_peerAddr,
									&rx->m_peerSockLen);
			
			if (nBytes == 0) {
				break;
			}
			if (nBytes < 0) {
				if (errno == EAGAIN) {
					break;
				}
				printf("ERROR: Unexpected receive error, got errno %d\n", errno);
				sleep(3);
				break;
			}
			if (nBytes != sizeof(WootPkt)) {
				continue;
			}
			if (pkt.header.magic != WOOT_PKT_MAGIC) {
				continue;
			}

			if (pkt.header.seq > lastSeq + 1) {
				printf("WARN: Seq skip %d - %d\n", lastSeq, pkt.header.seq);
			}
			else if (pkt.header.seq < lastSeq) {
				printf("WARN: Out of order seq %d - %d\n", lastSeq, pkt.header.seq);
			}
			lastSeq = pkt.header.seq;

			for (int i = 0; i < NETBUFF_SAMPLES; i++) {
				rx->m_buf[rx->m_writePos + i] = ((float)pkt.samples[i]) / 32768.0f;
			}
			rx->m_writePos += NETBUFF_SAMPLES;
			rx->m_writePos %= RINGBUFF_SAMPLES;
		}

		fflush(stdout);
		usleep(250);
	}

	printf("Ending rxUdp\n");
	fflush(stdout);
}

void WootRx::writeReceivedFrame(BelaContext *context, Mixer &mixer) {
	int bufSamples = m_writePos - m_readPos;
	if (bufSamples < 0) {
		bufSamples += RINGBUFF_SAMPLES;
	}
	if (bufSamples > (NETBUFF_SAMPLES * m_rxQueueSize)) {
		m_readPos += (RINGBUFF_SAMPLES + bufSamples) - (NETBUFF_SAMPLES * m_rxQueueSize);
		m_readPos %= RINGBUFF_SAMPLES;
	}

	mixer.addLayer();
	if (bufSamples >= NETBUFF_SAMPLES) {
		liquid_float_complex upsampled[2];
		liquid_float_complex sample;
		sample.imag = 0.0f;
		for(unsigned int n = 0; n < NETBUFF_SAMPLES; n++) {
			sample.real = m_buf[m_readPos + n];
			// upsample/interpolate from 22.05 back to 44.1
			resamp2_crcf_interp_execute(m_resampler, sample, upsampled);
			mixer.writeSample(n * 2, upsampled[0].real);
			mixer.writeSample(n * 2 + 1, upsampled[1].real);
			m_meter.feedSample(upsampled[0].real);
			m_meter.feedSample(upsampled[1].real);
		}
		m_readPos += NETBUFF_SAMPLES;
		m_readPos %= RINGBUFF_SAMPLES;
	}
}
