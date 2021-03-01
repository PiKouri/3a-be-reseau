#include <mictcp.h>
#include <api/mictcp_core.h>

#define TIMEOUT 10

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(10);

   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
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
    static int seqnum = 0; // Numéro de séquence, variable globale

    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    // Création du PDU

    mic_tcp_pdu pdu;
    pdu.payload.data = mesg;
    pdu.payload.size = mesg_size;
    pdu.header.source_port=0, pdu.header.dest_port=0, pdu.header.ack_num=0, pdu.header.syn=0, pdu.header.ack=0, pdu.header.fin=0; 
    pdu.header.seq_num=seqnum;

    // Création de l'adresse de destination

    mic_tcp_sock_addr addr;
    struct hostent * ip_addr = gethostbyname("localhost");
    addr.ip_addr = (*ip_addr).h_addr_list[0]; 
    addr.ip_addr_size = (*ip_addr).h_length;
    addr.port = 0;

    // Envoi du PDU

    IP_send(pdu, addr);

    // Attente du ACK, et renvoi si timeout
    mic_tcp_pdu ack;
    int ok = 0;
    ok = (IP_recv(&ack, &addr, TIMEOUT) != -1);
    while (!(ok)) {
        printf("[MIC-TCP] Renvoi du PDU %d\n", seqnum); // Permet de visualiser le STOP & Wait
        IP_send(pdu, addr);
        ok = (IP_recv(&ack, &addr, TIMEOUT) != -1);
        ok &= ack.header.ack == 1;
        ok &= ack.header.ack_num == seqnum;
    }
    printf("[MIC-TCP] Reception du ACK %d\n", ack.header.ack_num); // Permet de visualiser le STOP & Wait
    seqnum++;
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
    mic_tcp_payload p;
	p.data = mesg;
 	p.size = max_mesg_size;
	int get_size = app_buffer_get(p);
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
    return 0;
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

    // Ajout du message au buffer de réception

    app_buffer_put(pdu.payload);

    // Création du ACK

    mic_tcp_pdu ack;
    ack.payload.data = NULL;
    ack.payload.size = 0;
    ack.header.source_port=0, ack.header.dest_port=0, ack.header.seq_num=0, ack.header.syn=0, ack.header.fin=0; 
    ack.header.ack = 1;
    ack.header.ack_num = pdu.header.seq_num; // On envoie un ACK correspondant au message reçu (numéro de séquence correspondant)

    // Envoi du ACk
    printf("[MIC-TCP] Envoi du ACK %d\n", ack.header.ack_num); // Permet de visualiser le STOP & Wait
    IP_send(ack, addr);
}
