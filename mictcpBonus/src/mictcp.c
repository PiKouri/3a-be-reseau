#include <sys/socket.h>
#include <mictcp.h>
#include <api/mictcp_core.h>
#include <errno.h>

#define LOSS_RATE 5
#define PERTES_ADMISSIBLES 3
#define INFOS 1 // Affichage des informations
#define TIMEOUT 1

static mic_tcp_sock sock;

static int seqnum = 0; // Numéro de séquence, variable globale

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
        sock.fd = socket;
        sock.addr = addr;
        sock.state=CLOSED;
    return 0;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    if (INFOS) {printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");}
    if (sock.fd == socket) {
        sock.state = IDLE;
        return 0;
    } else return -1;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    if (INFOS) {printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");}
    sock.fd = socket;
    //memcpy((char*)&(sock.addr), (char *) (addr.ip_addr), addr.ip_addr_size);
    sock.addr = addr;
    sock.state=IDLE;

    int result = 0;
    mic_tcp_pdu syn;
    syn.payload.data = NULL;
    syn.payload.size = 0;
    syn.header.source_port=0, syn.header.dest_port=0, syn.header.seq_num=-1, syn.header.ack_num = -1,  syn.header.ack = 0, syn.header.fin=0; 
    syn.header.syn=1;

    mic_tcp_pdu synack;
    mic_tcp_sock_addr rcv_addr; 
    mic_tcp_pdu ack = syn;
    ack.header.ack = 1; ack.header.syn = 0;

    sock.state = SYN_SENT;
    while (!result) {
        IP_send(syn, addr);
        if (INFOS) printf("[INFOS] Envoi du SYN\n");
        result = (IP_recv(&synack, &rcv_addr, TIMEOUT) != -1);
        result &= (synack.header.syn == 1) && (synack.header.ack == 1);
        if (result) {
            if (INFOS) printf("[INFOS] Reception du SYNACK\n");
            if (INFOS) printf("[INFOS] Envoi du ACK\n");
            IP_send(ack, addr);
        }
    }
    sock.state = ESTABLISHED;
    return (result) ? 0 : -1;
}
/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    if (sock.fd == mic_sock) {

        static int compteur_pertes_effectif = 0;

        if (INFOS) {printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");}

        // Création du PDU

        mic_tcp_pdu pdu;
        pdu.payload.data = mesg;
        pdu.payload.size = mesg_size;
        pdu.header.source_port=0, pdu.header.dest_port=0, pdu.header.ack_num=0, pdu.header.syn=0, pdu.header.ack=0, pdu.header.fin=0; 
        pdu.header.seq_num=seqnum; 

        // Création de l'adresse de destination

        mic_tcp_sock_addr addr = sock.addr;

        // Envoi du PDU

        IP_send(pdu, addr);

        // Attente du ACK, et renvoi si timeout
        // V3 fiabilité partielle
        int pourcentage_recu;

        mic_tcp_pdu ack;
        int ok = 0;
        mic_tcp_sock_addr rcv_addr;
        ok = (IP_recv(&ack, &rcv_addr, TIMEOUT) != -1);
        pourcentage_recu = 100-(compteur_pertes_effectif*100/(seqnum+1));
        if (!ok && pourcentage_recu >= 100-PERTES_ADMISSIBLES) compteur_pertes_effectif++; // On compte que les pertes réelles, on ne compte pas les pdus renvoyés
        while (!(ok)) {
            if (INFOS) { // Permet de visualiser la fiabilité partielle
                printf("[Infos] Pertes %d/%d paquets soit %d%%/%d%% admissibles\n", compteur_pertes_effectif, seqnum+1, 100-pourcentage_recu, PERTES_ADMISSIBLES);
            }
            if (pourcentage_recu >= 100-PERTES_ADMISSIBLES) {
                if (INFOS) printf("[Infos] Pourcentage perte OK\n");
                break;
            }
            
            if (INFOS) printf("[Infos] Renvoi du PDU %d\n", seqnum); // Permet de visualiser le STOP & Wait
            IP_send(pdu, addr);
            ok = (IP_recv(&ack, &rcv_addr, TIMEOUT) != -1);
            ok &= ack.header.ack == 1;
            ok &= ack.header.ack_num == seqnum;
        }
        if (INFOS) if (ok) printf("[Infos] Reception du ACK %d\n", ack.header.ack_num); // Permet de visualiser le STOP & Wait
        seqnum++;
        return mesg_size; 

    } else return -1;
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    if (sock.fd == socket) {
        if (INFOS) {printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");}
        mic_tcp_payload p;
        p.data = mesg;
        p.size = max_mesg_size;
        int get_size = app_buffer_get(p);
        return get_size;
    } else return -1;

}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */ 
int mic_tcp_close (int socket)
{
    if (sock.fd == socket) {
        if (INFOS) {printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");}
        int result = 0;
        sock.state = CLOSING;
        mic_tcp_pdu fin;
        fin.payload.data = NULL;
        fin.payload.size = 0;
        fin.header.source_port=0, fin.header.dest_port=0, fin.header.seq_num=-1, fin.header.ack_num = -1,  fin.header.ack = 0, fin.header.syn=0; 
        fin.header.fin=1;

        mic_tcp_pdu finack;
        mic_tcp_sock_addr rcv_addr; 
        mic_tcp_pdu ack = fin;
        ack.header.ack = 1; ack.header.fin = 0;
        
        while (!result) {
            IP_send(fin, sock.addr);
            if (INFOS) printf("[INFOS] Envoi du FIN\n");
            result = (IP_recv(&finack, &rcv_addr, TIMEOUT) != -1);
            result &= (finack.header.fin == 1) && (finack.header.ack == 1);
            if (result) {
                if (INFOS) printf("[INFOS] Reception du FINACK\n");
                if (INFOS) printf("[INFOS] Envoi du ACK\n");
                IP_send(ack, sock.addr);
            }
        }
        sock.state = CLOSED;
        return 0;
    } else return -1;
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
    int result;
    mic_tcp_pdu synack;
    synack.payload.data = NULL;
    synack.payload.size = 0;
    synack.header.source_port=0, synack.header.dest_port=0, synack.header.seq_num=-1, synack.header.ack_num = -1, synack.header.fin=0; 
    synack.header.ack = 1; synack.header.syn = 1;
    mic_tcp_pdu finack = synack;
    finack.header.fin = 1; finack.header.syn = 0;

    if (sock.state == ESTABLISHED) {
        if (pdu.header.fin == 1) {
            if (INFOS) printf("[INFOS] Reception du FIN\n");
            sock.state = CLOSING;
            if (INFOS) printf("[INFOS] Envoi du FINACK\n");
            IP_send(finack, addr);
        } else {
            int pid = fork();
            switch (pid) {
                case -1:
                    printf("\n----------- /!\\ Echec lors du fork /!\\ -----------\n\n");
                    exit(1);
                case 0: // On est dans le proc. fils
                    if (INFOS) {printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");}

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
                    seqnum=pdu.header.seq_num;
                    exit(1);
                default: // On est dans le proc. père

                    // Ajout du message au buffer de réception
                    
                    if (pdu.header.seq_num == seqnum+1 || pdu.header.seq_num == 0) {
                        app_buffer_put(pdu.payload);
                    } 
                    seqnum=pdu.header.seq_num;
                    break;
            }
        }

    } else if (sock.state == IDLE) {
        result = (pdu.header.syn == 1) && (pdu.header.ack == 0);
        if (result) {
            if (INFOS) printf("[INFOS] Reception du SYN\n");
            sock.state = SYN_RECEIVED;
            sock.addr = addr;
            if (INFOS) printf("[INFOS] Envoi du SYNACK\n");
            IP_send(synack, addr);
        }
    } else if (sock.state == SYN_RECEIVED) {
        result = (pdu.header.syn == 0) && (pdu.header.ack == 1);
        if (result && INFOS) printf("[INFOS] Reception du ACK\n");
        if (result) sock.state = ESTABLISHED;
        else {
            if (INFOS) printf("[INFOS] Envoi du SYNACK\n");
            IP_send(synack, addr);
        }
    } else if (sock.state == CLOSING) {
        result = (pdu.header.fin == 0) && (pdu.header.ack == 1);
        if (result && INFOS) printf("[INFOS] Reception du ACK\n");
        if (result) {
            sock.state = CLOSED;
            exit(1);
        }
        else {
            if (INFOS) printf("[INFOS] Envoi du FINACK\n");
            IP_send(finack, addr);
        }
    }
} 
