# 3IR BE RESEAU
## Avancement 
v1, v2, v3 (fiabilité partielle sans fenêtre glissante)

## Commandes 
./tsock_video -p -t mictcp et ./tsock_video -s -t mictcp

## Informations 
Les #define permet d'ajuster les paramètres de mictcp <br>
  * LOSS_RATE : pourcentage de perte passé en paramètre à la fonction set_loss_rate()
  * PERTES_ADMISSIBLES : pourcentage de perte admissible
  * INFOS : Affiche les informations (notamment les ACKs) si =1 et cache les informations si =0
  * TIMEOUT : Délai avant timeout pour la fonction IP_rcv()

## Bugs
  * mictcpV3 : bug lorsqu'on lance la source avant le puit, le lecteur ne se lance qu'après quelques secondes (~ PDU 900)
  * mictcpBonus : bug non réparé : parfois l'établissement de connexion reste bloqué sur "Envoi du SYN" | "Envoi du SYNACK" et pareil pour la fermeture sur "ENVOI du FIN" ... Essayer de lancer quasi-simultanément la source et le puit
