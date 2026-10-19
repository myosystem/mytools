#include <myos>
#include <net>
#include <string.h>
#include <string>
#include <stdlib.h>
struct dhcp_header {
    uint8_t  op;          // 1=BOOTREQUEST(클라이언트), 2=BOOTREPLY(서버)
    uint8_t  htype;       // 1 = 이더넷
    uint8_t  hlen;        // 6 = MAC 길이
    uint8_t  hops;        // 0
    uint32_t xid;         // 트랜잭션 ID. 응답에 그대로 돌아옴
    uint16_t secs;        // 0
    uint16_t flags;       // 0x8000 = 응답을 브로드캐스트로
    uint32_t ciaddr;      // 내 현재 IP (없으면 0)
    uint32_t yiaddr;      // 서버가 채워주는 "너한테 줄 IP"
    uint32_t siaddr;      // 다음 부트서버. 무시
    uint32_t giaddr;      // 릴레이. 0
    uint8_t  chaddr[16];  // 앞 6바이트에 내 MAC, 나머지 0
    uint8_t  sname[64];   // 0
    uint8_t  file[128];   // 0
    uint32_t magic;       // 0x63825363
} __attribute__((packed));

#define DHCP_MAGIC        0x63825363

// op
#define BOOTREQUEST       1
#define BOOTREPLY         2

// 옵션 53 값 (메시지 종류)
#define DHCPDISCOVER      1
#define DHCPOFFER         2
#define DHCPREQUEST       3
#define DHCPDECLINE       4
#define DHCPACK           5
#define DHCPNAK           6
#define DHCPRELEASE       7

// 옵션 코드
#define OPT_SUBNET_MASK   1
#define OPT_ROUTER        3
#define OPT_DNS           6
#define OPT_REQUESTED_IP  50
#define OPT_LEASE_TIME    51
#define OPT_MSG_TYPE      53
#define OPT_SERVER_ID     54
#define OPT_PARAM_LIST    55
#define OPT_END           255

// 포트
#define DHCP_CLIENT_PORT  68
#define DHCP_SERVER_PORT  67

// 수신 버퍼 크기
#define DHCP_BUF_SIZE     4080
// 응답 대기 시간
#define DHCP_TIMEOUT_MS   4000
static void put_option(std::string& packet) {
    uint8_t raw[] = {
        OPT_MSG_TYPE,   1, DHCPDISCOVER,
        OPT_PARAM_LIST, 3, OPT_SUBNET_MASK, OPT_ROUTER, OPT_DNS,
        OPT_END
    };
    packet.append((const char*)raw, sizeof(raw));
}
static const uint8_t* find_opt(const uint8_t* buf, int size, uint8_t code, uint8_t* len) {
    int i = sizeof(dhcp_header);
    while (i < size) {
        uint8_t c = buf[i];
        if (c == OPT_END) return nullptr;
        if (c == 0) { i++; continue; }          // 패딩은 길이 바이트가 없음
        if (i + 1 >= size) return nullptr;      // 길이 바이트가 잘림
        uint8_t l = buf[i + 1];
        if (i + 2 + l > size) return nullptr;   // 값이 버퍼 밖
        if (c == code) { *len = l; return &buf[i + 2]; }
        i += 2 + l;
    }
    return nullptr;
}
static void put_option_request(std::string& packet, uint32_t req_ip, uint32_t server_id) {
    uint8_t raw[] = {
        OPT_MSG_TYPE,   1, DHCPREQUEST,
        OPT_REQUESTED_IP, 4, 0,0,0,0,        // 아래에서 채움
        OPT_SERVER_ID,    4, 0,0,0,0,
        OPT_PARAM_LIST,   3, OPT_SUBNET_MASK, OPT_ROUTER, OPT_DNS,
        OPT_END
    };
    uint32_t a = htonl(req_ip), b = htonl(server_id);
    memcpy(&raw[5], &a, 4);
    memcpy(&raw[11], &b, 4);
    packet.append((const char*)raw, sizeof(raw));
}
static bool wait_dhcp(Socket& sock, uint32_t xid, uint8_t want, uint8_t* buf, uint16_t* out_size, uint64_t timeout_ms) {
    uint64_t timer = set_timer(timeout_ms, 0);
    while (true) {
        wait_for_msg();
        msg_t m;
        while (receive_msg(&m) == 0) {
            if (m.type == MSG_TIMER_EXPIRED) { return false; }
            if (m.type != MSG_SOCKET_DATA)   continue;

            uint16_t size = DHCP_BUF_SIZE;
            while (sock.recv(buf, &size) > 0) {          // 큐 빌 때까지
                if (size < sizeof(dhcp_header)) { size = DHCP_BUF_SIZE; continue; }
                dhcp_header* r = (dhcp_header*)buf;
                if (r->op == BOOTREPLY && r->xid == htonl(xid)
                    && r->magic == htonl(DHCP_MAGIC)) {
                    uint8_t l;
                    const uint8_t* t = find_opt(buf, size, OPT_MSG_TYPE, &l);
                    if (t && l == 1 && *t == want) {
                        cancel_timer(timer);
                        *out_size = size;
                        return true;
                    }
                }
                size = DHCP_BUF_SIZE;
            }
        }
    }
}
int main() {
    size_t ncount = Net::Link::count();
    for (int i = 0; i < ncount; i++) {
        if (Net::Addr::get(i) != 0) continue;
        uint8_t mac[6];
        Net::Link::mac(i, mac);
        uint32_t xid = (uint32_t)get_cycles() ^ (mac[5] | (mac[4] << 8));
        dhcp_header header;
        memset(&header, 0, sizeof(header));
        header.op = BOOTREQUEST;
        header.htype = 1;                       // 이더넷
        header.hlen = 6;
        header.hops = 0;
        header.xid = htonl(xid);
        header.secs = 0;
        header.flags = htons(0x8000);           // 응답을 브로드캐스트로
        header.ciaddr = 0;
        header.yiaddr = 0;
        header.siaddr = 0;
        header.giaddr = 0;
        memcpy(header.chaddr, mac, 6);
        header.magic = htonl(DHCP_MAGIC);
        std::string packet;
        packet.append((const char*)&header, sizeof(header));
        put_option(packet);
        if (packet.size() < 300) packet.resize(300);
        Socket socket(SocketType::UDP, 68);
        socket.set_option(SOCK_OPT_MESSAGE);
        socket.set_device(i);
        socket.connect(-1, 67);
        socket.send(packet.c_str(), packet.size());
        uint8_t* recv_buf = (uint8_t*)malloc(DHCP_BUF_SIZE);
        dhcp_header* r = nullptr;
        uint16_t size = 0;
        uint8_t l;
        const uint8_t* p;

        // --- OFFER 대기: REQUEST에 필요한 값만 뽑는다 ---
        if (!wait_dhcp(socket, xid, DHCPOFFER, recv_buf, &size, DHCP_TIMEOUT_MS)) {
            free(recv_buf);
            continue;
        }
        r = (dhcp_header*)recv_buf;
        uint32_t offered_ip = ntohl(r->yiaddr);
        uint32_t server_id = 0;
        p = find_opt(recv_buf, size, OPT_SERVER_ID, &l);
        if (!p || l != 4) { free(recv_buf); continue; }          // 서버 식별자는 필수
        memcpy(&server_id, p, 4); server_id = ntohl(server_id);

        // --- REQUEST 전송 (xid는 그대로, 헤더 재사용) ---
        std::string req;
        req.append((const char*)&header, sizeof(header));
        put_option_request(req, offered_ip, server_id);
        if (req.size() < 300) req.resize(300);
        socket.send(req.c_str(), req.size());

        // --- ACK 대기: 최종 확정값은 여기서 읽는다 (OFFER와 다를 수 있음) ---
        if (!wait_dhcp(socket, xid, DHCPACK, recv_buf, &size, DHCP_TIMEOUT_MS)) {
            free(recv_buf);
            continue;
        }
        r = (dhcp_header*)recv_buf;
        uint32_t my_ip = ntohl(r->yiaddr);
        uint32_t mask = 0, gateway = 0;

        p = find_opt(recv_buf, size, OPT_SUBNET_MASK, &l);
        if (p && l == 4) { memcpy(&mask, p, 4); mask = ntohl(mask); }

        p = find_opt(recv_buf, size, OPT_ROUTER, &l);
        if (p && l >= 4) { memcpy(&gateway, p, 4); gateway = ntohl(gateway); }

        free(recv_buf);

        // --- 적용 ---
        Net::Addr::set(i, my_ip);
        Net::Route::del(i);                                      // 이전 라우트 정리
        if (mask)    Net::Route::add(my_ip & mask, mask, 0, i);  // 온링크
        if (gateway) Net::Route::add(0, 0, gateway, i);          // 기본
    }
}