#ifndef REMOTE_UDP_LOGGING_H
#define REMOTE_UDP_LOGGING_H

static UdpConnection remote_udp_log_socket;
static IPAddress remote_udp_log_ip;
static uint16_t remote_udp_log_port;

char remote_udp_log_buf[320];
char* remote_udp_log_ptr = NULL;
void remote_udp_log_putc(uart_t * dummy, char c) {
    if (remote_udp_log_ptr == NULL) remote_udp_log_ptr = remote_udp_log_buf;
    *(remote_udp_log_ptr++) = c;
    if (c == '\n') {
        remote_udp_log_ptr += sprintf(remote_udp_log_ptr, "%012d\t", RTC.getRtcSeconds());
    }
    if (remote_udp_log_ptr - remote_udp_log_buf >= 300) {
        *remote_udp_log_ptr = '\0';
        remote_udp_log_socket.sendStringTo(remote_udp_log_ip,remote_udp_log_port,remote_udp_log_buf);
        remote_udp_log_ptr = remote_udp_log_buf;
    }
}

//set m_printf callback
extern void setMPrintfPrinterCbc(void (*callback)(uart_t *, char), uart_t *uart);


void remote_udp_log_enable(IPAddress targetIp, uint16_t targetPort) {
    debug_i("Enabling remote UDP logging, destination=%s:%d", targetIp.toString().c_str(), targetPort);
    remote_udp_log_ip = targetIp;
    remote_udp_log_port = targetPort;
    setMPrintfPrinterCbc(remote_udp_log_putc, NULL);
    debugf("------------------------------"); // this goes out via remote UDP logging
    debugf("remote_udp_log enabled"); // this goes out via remote UDP logging
	rst_info* info = system_get_rst_info();
    debugf("boot reason %d", info->reason); // this goes out via remote UDP logging
}
#endif