Stamate Robert Andrei
325CC
Tema 2 PCom
_______________________________________________________________________________
subscriber.cc

Clientul TCP implementat comunica cu server-ul cu ajutorul unor structuri:
"simple_message" (spre server) si "complex_message" (de la server).

Conectarea clientului la server se face folosind functia connect(). Aceasta
primeste ca parametru descriptorul de fisiere al server-ului, obtinut in urma
apelului functiei socket().

Dupa conectarea la server aplicatia se foloseste de multiplexarea I/O pentru
a inregistra mesajele date in consola (cu scopul de a le trimite la server) si
asteapta pe un socket de primire mesaje de la server.

-------------------------------------------------------------------------------
Trimitere:
Comanda subscribe: se genereaza un mesaj in format "subscribe <TOPIC> <SF>"in 
vederea abonarii la un topic;

Comanda unsubscribe: se genereaza un mesaj in format "unsubscribe <TOPIC>" in
vederea dezabonarii de la un topic;

Comanda exit: se inchide clientul cu exit(0) ("deconectarea propriu-zisa este
tratata direct in server").
-------------------------------------------------------------------------------
Primire: 
Se primeste cu functia recv un mesaj "complex" printr-o structura
caracterizata in cerinta:
<IP_CLIENT_UDP>:<PORT_CLIENT_UDP> - <TOPIC> - <TIP_DATE> - <VALOARE_MESAJ>
_______________________________________________________________________________

server.cc

Server-ul primeste date de la un client UDP in format <TOPIC>|<TIP>|<CONTINUT>
si comunica (I/O) cu un client TCP.

Socketurile UDP si TCP sunt asociate masinii locale folosind functia bind().
-------------------------------------------------------------------------------
Logica continua a server-ului este executata intr-un loop:

tcpSock:
Server-ul "asculta" pe un socket (tcpSock) toate cererile de conexiune si le
trateaza astfel: Clientii noi sunt adaugati intr-un dictionar (socket -> id),
iar clientii al caror ID exista deja in dictionar sunt ignorati. 
Se observa ca atat socketul cat si id-ul unui client sunt unice => un singur
dictionar este suficient.

STDIN_FILENO:
Singura comanda acceptata de server este "exit" -> dupa ce se inchid clientii
se inchide si server-ul; Alte comenzi sunt ignorate.

udpSock:
Sunt receptionate mesaje in forma Topic|Tip|Continut. Topicul si tipul pot fi
utilizate bit cu bit, iar continutul este parsat conform tabelului din cerinta,
dupa <TIP>. In final este generat un mesaj "complex" care se trimite tuturor
clientilor conectati, abonati la topicul <TOPIC>.

socket client:
Sunt receptionate mesaje de la client formatate "subscribe <TOPIC> <SF>" sau
"unsubscribe <TOPIC>".

Comanda subscribe: intr-un dictionar <STRING(TOPIC), VECTOR<STRING(ID)>> se
adauga clienti in functie de id-ul unic.

Comanda unsubscribe: clientul cu id "ID" este sters din dictionar.
_______________________________________________________________________________