#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 12345
#define CLIENT_PORT 54321
#define SERVER_IP "127.0.0.1"

void print_tcp_flags(struct tcphdr *tcp) {
    std::cout << "[+] TCP Flags: "
              << " SYN: " << tcp->syn
              << " ACK: " << tcp->ack
              << " FIN: " << tcp->fin
              << " RST: " << tcp->rst
              << " PSH: " << tcp->psh
              << " SEQ: " << ntohl(tcp->seq) << std::endl;
}

void send_syn(int sock, struct sockaddr_in *server_addr) {
    char packet[sizeof(struct iphdr) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));

    struct iphdr *ip = (struct iphdr *)packet;
    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));

    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(packet));
    ip->id = htons(12345);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = inet_addr("127.0.0.1");
    ip->daddr = server_addr->sin_addr.s_addr;

    tcp->source = htons(CLIENT_PORT);
    tcp->dest = htons(SERVER_PORT);
    tcp->seq = htonl(200);
    tcp->ack_seq = 0;
    tcp->doff = 5;
    tcp->syn = 1;
    tcp->ack = 0;
    tcp->window = htons(8192);
    tcp->check = 0;

    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto() failed");
    } else {
        std::cout << "[+] Sent SYN with SEQ=200" << std::endl;
    }
}

void send_ack(int sock, struct sockaddr_in *server_addr, struct tcphdr *received_tcp) {
    char packet[sizeof(struct iphdr) + sizeof(struct tcphdr)];
    memset(packet, 0, sizeof(packet));

    struct iphdr *ip = (struct iphdr *)packet;
    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));

    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(sizeof(packet));
    ip->id = htons(12347);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = inet_addr("127.0.0.1");
    ip->daddr = server_addr->sin_addr.s_addr;

    tcp->source = htons(CLIENT_PORT);
    tcp->dest = htons(SERVER_PORT);
    tcp->seq = htonl(600);
    tcp->ack_seq = htonl(ntohl(received_tcp->seq) + 1);
    tcp->doff = 5;
    tcp->syn = 0;
    tcp->ack = 1;
    tcp->window = htons(8192);
    tcp->check = 0;

    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("sendto() failed");
    } else {
        std::cout << "[+] Sent ACK with SEQ=600" << std::endl;
    }
}

int main() {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    send_syn(sock, &server_addr);

    char buffer[65536];
    struct sockaddr_in source_addr;
    socklen_t addr_len = sizeof(source_addr);
    bool handshake_complete = false;

    while (!handshake_complete) {
        int data_size = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&source_addr, &addr_len);
        if (data_size < 0) {
            perror("Packet reception failed");
            continue;
        }

        struct iphdr *ip = (struct iphdr *)buffer;
        struct tcphdr *tcp = (struct tcphdr *)(buffer + (ip->ihl * 4));

        if (ntohs(tcp->source) != SERVER_PORT || ntohs(tcp->dest) != CLIENT_PORT) {
            continue;
        }

        if (tcp->syn == 1 && tcp->ack == 1 && ntohl(tcp->seq) == 400 && ntohl(tcp->ack_seq) == 201) {
            std::cout << "[+] Received SYN-ACK from server" << std::endl;
            print_tcp_flags(tcp);
            
            send_ack(sock, &server_addr, tcp);
            handshake_complete = true;
        }
    }

    std::cout << "[+] TCP handshake completed successfully" << std::endl;
    close(sock);
    return 0;
}
