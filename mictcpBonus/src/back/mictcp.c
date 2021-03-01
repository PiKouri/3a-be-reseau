#include <mictcp.h>
#include <api/mictcp_core.h>

#define LOSS_RATE 2
#define PERTES_ADMISSIBLES 5
#define INFOS 1 // Affichage des informations
#define TIMEOUT 5000

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   if (INFOS) {printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");}
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(LOSS_RATE);

   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   if (INFOS) {printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");}

   /*if (bind(socket, (const struct sockaddr *) &addr, sizeof(addr)) == -1) {
       perror("Echec du bind()");
       exit(1);
   }*/

   return 0;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    if (INFOS) {printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");}
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    if (INFOS) {printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");}
    return 0;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    static int seqnum = 0; // Numéro de séquence, variable globale
    static int compteur_pertes_effectif = 0;

    if (INFOS) {printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");}

    // Création du PDU

    mic_tcp_pdu pdu;
    pdu.payload.data = mesg;
    pdu.payload.size = mesg_size;
    pdu.header.source_port=0, pdu.header.dest_port=0, pdu.header.ack_num=0, pdu.header.syn=0, pdu.header.ack=0, pdu.header.fin=0; 
    pdu.header.seq_num=seqnum; // A vérifier

    // Création de l'adresse de destination

    mic_tcp_sock_addr addr;
    struct hostent * ip_addr = gethostbyname("localhost");
    addr.ip_addr = (*ip_addr).h_addr_list[0]; 
    addr.ip_addr_size = (*ip_addr).h_length;
    addr.port = 0;

    // Envoi du PDU

    IP_send(pdu, addr);

    // Attente du ACK, et renvoi si timeout
    // V3 fiabilité partielle
    int pourcentage_recu;

    mic_tcp_pdu ack;
    int ok = 0;
    ok = (IP_recv(&ack, &addr, TIMEOUT) != -1);
    if (!ok) compteur_pertes_effectif++;
    while (!(ok)) {

        pourcentage_recu = 100-(compteur_pertes_effectif*100/seqnum);
        if (INFOS) { // Permet de visualiser la fiabilité partielle
            printf("[Infos] Pertes %d/%d paquets soit %d%%/%d%% admissibles\n", compteur_pertes_effectif, seqnum, 100-pourcentage_recu, PERTES_ADMISSIBLES);
        }
        if (pourcentage_recu >= 100-PERTES_ADMISSIBLES) {
            if (INFOS) printf("[Infos] Pourcentage perte OK\n");
            break;
        }
        
        if (INFOS) printf("[Infos] Renvoi du PDU %d\n", seqnum); // Permet de visualiser le STOP & Wait
        IP_send(pdu, addr);
        ok = (IP_recv(&ack, &addr, TIMEOUT) != -1);
        ok &= ack.header.ack == 1;
        ok &= ack.header.ack_num == seqnum;
    }
    if (INFOS) if (ok) printf("[Infos] Reception du ACK %d\n", ack.header.ack_num); // Permet de visualiser le STOP & Wait
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
    if (INFOS) {printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");}
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
    if (INFOS) {printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");}
    return shutdown(socket, 2);
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * 
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr)
{
    if (INFOS) {printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");}

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
    if (INFOS) printf("[Infos] Envoi du ACK %d\n", ack.header.ack_num); // Permet de visualiser le STOP & Wait
    IP_send(ack, addr);
}
