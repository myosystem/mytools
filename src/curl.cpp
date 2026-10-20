#include <unistd.h>
#include <myos>
#include <stdlib.h>
#include <vector>
#include <string>
// 응답이 올 때까지 기다린다. 시간 초과면 false.
static bool wait_reply(Socket& sock, uint8_t* buf, uint16_t* len, uint64_t timeout_ms) {
	uint16_t cap = *len;
	uint64_t timer = set_timer(timeout_ms, 0);
	while (true) {
		wait_for_msg();
		msg_t m;
		while (receive_msg(&m) == 0) {
			if (m.type == MSG_TIMER_EXPIRED) return false;
			if (m.type != MSG_SOCKET_DATA)   continue;
			*len = cap;
			if (sock.recv(buf, len) >= 0) {
				cancel_timer(timer);
				return true;
			}
		}
	}
}
int parse_ipv4(char** argv, uint32_t* result) {
	char* ip[4];
	ip[0] = argv[1];
	int n = 1;
	for (int i = 0; argv[1][i] != 0; i++) {
		if (argv[1][i] == '.') {
			if (n >= 4) {
				return 1;
			}
			ip[n++] = argv[1] + (i + 1);
		}
	}
	if (n != 4) {
		return 1;
	}
	for (int i = 0; i < 4; i++) {
		if (!('0' <= *ip[i] && *ip[i] <= '9')) {
			return 1;
		}
	}
	*result = ipaddr(atoi(ip[0]), atoi(ip[1]), atoi(ip[2]), atoi(ip[3]));
	return 0;
}
struct dns_header {
	uint16_t id;        // 임의값, 응답이랑 매칭용 (그대로 echo됨)
	uint16_t flags;
	uint16_t qdcount;   // 질문 개수
	uint16_t ancount;   // 응답 레코드 개수 (쿼리 보낼 땐 0)
	uint16_t nscount;   // authority 레코드 개수 (쿼리 보낼 땐 0)
	uint16_t arcount;   // additional 레코드 개수 (쿼리 보낼 땐 0)
} __attribute__((packed));
void skip_name(uint8_t*& pointer) {
	while (1) {
		if ((*pointer & 0xC0) == 0xC0) {
			pointer += 2;
			break;
		}
		if (*pointer == 0) {
			pointer++;
			break;
		}
		pointer += 1 + *pointer;
	}
}
int parse_domain(char** argv, uint32_t* result) {
	std::vector<std::string> domains;
	std::string domain = "";
	for (int i = 0;; i++) {
		if (argv[1][i] == '.') {
			domains.push_back(domain);
			printf("%s\n", domain.c_str());
			domain = "";
			continue;
		}
		if (argv[1][i] == '\0') {
			domains.push_back(domain);
			printf("%s\n", domain.c_str());
			domain = "";
			break;
		}
		domain.push_back(argv[1][i]);
	}
	dns_header dns;
	dns.id = htons(0xFF);
	dns.flags = htons(0x0100);
	dns.qdcount = htons(1);
	dns.ancount = 0;
	dns.nscount = 0;
	dns.arcount = 0;
	domain.append((char*)&dns, sizeof(dns));
	for (auto& str : domains) {
		if (str.size() == 0) return 1;
		uint8_t size = str.size();
		domain.append((char*)&size, sizeof(size));
		domain += str;
	}
	domain.push_back(0);
	uint16_t qtype = htons(1);
	domain.append((char*)&qtype, sizeof(qtype));
	uint16_t qclass = htons(1);
	domain.append((char*)&qclass, sizeof(qclass));
	Socket socket(SocketType::UDP);
	socket.set_option(SOCK_OPT_MESSAGE);
	socket.connect(ipaddr(8, 8, 8, 8), 53);
	socket.send(domain.c_str(), domain.size());
	uint8_t buf[512];
	uint16_t size = 512;
	if (!wait_reply(socket, buf, &size, 4000)) {
		printf("DNS timed out\n");
		return 1;
	}
	uint8_t* pointer = buf;
	pointer += sizeof(dns_header);
	skip_name(pointer);
	pointer += 4;
	skip_name(pointer);
	pointer += 10;
	*result = ntohl(*(uint32_t*)pointer);
	return 0;
}
int main(int argc, char** argv) {
	if (argc != 2) {
		printf("curl <IPv4>\n");
		return 1;
	}
	uint32_t ip;
	int result = parse_ipv4(argv, &ip);
	if (result == 1) {
		result = parse_domain(argv, &ip);
	}
	if (result != 0) {
		return result;
	}
	Socket socket(SocketType::TCP);
	socket.connect(ip, 80);
	std::string str = "GET / HTTP/1.1\r\nHost: ";
	str.append(argv[1]);
	str.append("\r\nConnection: close\r\n\r\n");
	socket.send(str.c_str(), str.size());
	uint8_t buf[512] = "";
	uint16_t size = sizeof(buf) - 1;
	while (socket.recv(buf, &size) > 0) {
		write(2, buf, size);
		size = sizeof(buf) - 1;
	}
	write(2, buf, size);
	return 0;
}
