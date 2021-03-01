#include <mictcp.h>
#include <api/mictcp_core.h>

int glob_seq_num = 0;
/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(0);

   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */

int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    //result = bind(socket, addr.ip_addr, addr.ip_addr_size);  --erreur de cast, à modifier ultérieurement
    return 0;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    //set_loss_rate(0);

    mic_tcp_pdu pdu;
    //Mise à jour des champs du PDU
    pdu.payload.data = mesg, pdu.payload.size = mesg_size;
    pdu.header.ack =0, pdu.header.ack_num = NULL, pdu.header.dest_port = 0, pdu.header.fin = 0, pdu.header.seq_num = glob_seq_num++, pdu.header.source_port = 0, pdu.header.syn = 0;
    struct hostent * hp = gethostbyname("localhost");
    mic_tcp_sock_addr addr;
    memcpy (&(addr.ip_addr), hp->h_addr, hp->h_length); // <======
    addr.ip_addr_size = sizeof(addr.ip_addr), addr.port = 0;
    IP_send(pdu, addr);

    mic_tcp_pdu ack;
    unsigned long timeout = 1;
    int ack_rcvd = 0;
    
    while (!ack_rcvd) {
        int res = -1;
        res = IP_recv(&ack, &addr, timeout);
        if (res != -1) {
            ack_rcvd = 1;
            //printf("[MIC-TCP] ACK numéro %d reçu\n", ack.header.ack_num);
        } else {
            IP_send(pdu, addr);
            //printf("[MIC-TCP] PACKET numéro %d ré-envoyé\n", pdu.header.seq_num);
        }
    }
    return mesg_size;
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_payload pdu;
    pdu.data = mesg;
    pdu.size = max_mesg_size;
    int get_size = app_buffer_get(pdu);
    return get_size;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    int result = shutdown(socket, 2);
    return result;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    app_buffer_put(pdu.payload);
    mic_tcp_pdu ack;
    //Mise à jour des champs du ACK
    ack.header.ack = 1, ack.header.syn = 0, ack.header.fin = 0;
    ack.header.ack_num = pdu.header.seq_num, ack.header.seq_num = NULL;
    ack.header.dest_port = pdu.header.source_port, ack.header.source_port = pdu.header.dest_port;
    ack.payload.data = NULL, ack.payload.size = NULL;
    IP_send(ack, addr);
    /* Vérification */
    //printf("[MIC-TCP] ACK numéro %d envoyé\n", ack.header.ack_num);
}
