# Rapport du projet de SY5

## I - Introduction

L'objectif de ce projet était de programmer un exécuteur de tâches via un couple client/démon. 
Le client Cassini va envoyer des requêtes au démon Saturnd qui tournera en tâche de fond. 
Dans ce rapport, nous traiterons tout d'abord de l'architecture de nos processus. 
Ensuite, nous expliquerons la manière dont nous avons stocké les tâches.
Nous détaillerons ensuite les différentes requêtes que l'on peut envoyer à Saturnd. 
Enfin, nous nous intéresserons aux potentielles erreurs de synchronisation que l'on peut rencontrer, ainsi qu'à des idées de fonctionnalités supplémentaires non implémentées.
Il s'agit d'un rapport long, mais nous avons préféré tout détailler, afin de ne pas oublier de choses importantes.


## II - Architecture
La gestion des requêtes du client cassini a pu se faire grâce à l'implémentation de saturnd, et avant d'en arriver à là, plusieurs contraintes se présentaient. 

Pour commencer, pour implémenter saturnd. 
L'idée était que la gestion des requêtes nécessitait de lire dans le fichier fifo de requête l'opération envoyée par cassini puis de récupérer les données nécessaires associés à cette même opération, c'est après qu'on se sera occupé de nos données que saturnd se chargera d'envoyer sa réponse (avec les conventions en big endian) à cassini par l'intermédiaire du fichier fifo de réponse que cassini lira.

### A - Gestion simultanée des requêtes et des exécutions de tâches

Le premier problème a été de réfléchir à une manière d'implémenter un moyen de, d'une part gérer les requêtes de Cassini et d'une autre part permettre l'exécution des tâches.
On s'est dit que, si saturnd devait exécuter toutes les tâches à un temps donné et qu'à ce même moment (ou juste un peu avant) une requête plus ou moins longue de cassini lui était parvenue, alors celui-ci se chargerait de gérer la requête de cassini ce qui aurait eu comme risque de retarder l'exécution de nos différentes tâches créées.

On a donc opté pour l'idée de diviser en 2 processus le fait de gérer les requêtes de cassini et le fait de gérer les exécutions des tâches chaque minutes.
L'un des processus s'occupera de la gestion des requêtes. Tandis que le second se chargerait de la création d'un fils temporaire à chaque minute qui se chargerait de l'exécution des tâches à exécuter à ce temps, ce fils sera pris en charge pour éviter la subsistance de tout processus zombie.

### B - Gestion de la terminaison du démon
Ensuite, il nous a semblé important de trouver un moyen de gérer la requête de terminaison de notre processus saturnd.

Nous avions au préalable opté de gérer cette requête par l'intermédiaire d'un pipe anonyme dont l'écriture se ferait lorsque saturnd reçoit la requête, et dont la lecture se ferait par le processus fils de saturnd, celui gérant l'exécution des tâches. 
Le problème était que pour exécuter les tâches, on avait notre processus qui faisait un sleep pendant une minute correspondant au timer avant laquelle celui-ci se chargerait de créer son processus fils qui se chargera alors de l'exécution des tâches.
Cependant, cela créait aussi un soucis dans le fait que la terminaison de notre processus ne se ferait qu'après notre sleep si le message envoyé dans le pipe est fait pendant ou dès que le processus va dans cette état. 
Nous avons donc changé de stratégie et prioriser l'utilisation de signaux, et d'un appel à pause à la place d'un sleep par notre processus.
Cela nous a alors permis de remplacer le sleep, par une alarme qui enverrai le signal `SIGALRM` chaque minute, et la gestion de la requête de terminaison se ferait dorénavant par un signal `SIGTERM` envoyé au processus de gestion de l'exécution des tâches, qui enlèverait ses fils zombies et exit  correctement (un `SIGKILL` serait trop brutal et risquerait de laisser place à des données potentiellement partiellement complétés). L'envoi d'un signal d'un processus père à ses fils étant plus simple que l'inverse, nous avons décidé que le père prendrait en charge l'exécution des commandes, et le fils l'exécution des tâches. 
Le gestionnaire des requêtes terminera correctement après l'exit de son processus fils.

### C - Récupération de l'exit code
L'exécution simple d'une tâche ne suffisait pas à gérer les requêtes nous permettant de récupérer l'exit code de notre exécution.
Il nous a alors fallu encore séparer le processus d'exécution des tâches en 2 processus, le fils qui se chargera de l'exécution de la commande et le processus père qui attendra l'exécution de son fils afin de récupérer son statut et d'alors finalement mettre à jour nos données.

Nous avons donc un total de 3 processus principaux, en tout cas ceux qui subsisteront au cours de l'exécution de saturnd :
Le 1er processus que l'on va qui se chargera des requêtes de cassini.
Le 2nd qui est le fils du premier qui se chargera du timer, et sensible aux signaux `SIGTERM` et `SIGALRM`.
Le 3ème processus qui est le fils du second, qui se chargera d'itérer dans nos tâches. Ce processus est terminé et recréé toutes les minutes, ce qui le rend moins subsistant au cours de l'exécution de ses parents.

C'est pendant cette itération qu'il vérifiera que l'on puisse bien exécuter notre tâche, et lorsque c'est le cas celui-ci mettra en place le processus exécuteur de tâche qui d'une part va lui même créer un processus qui va utiliser la commande exec correspondant à la commande de la tâche, et qui d'autre part attendra l'exit code de son processus exécuteur.
À noter que chaque processus prend en charge ses fils, donc à l'issue de nos exécutions il n'y aura pas d'accumulation de processus zombies.

Un [diagramme représentant l'architecture des processus](https://gaufre.informatique.univ-paris-diderot.fr/jeanmari/sy5-projet-2021-2022/blob/master/Diagramme_Saturnd.pdf) se trouve à la racine du dépot.

## III - Les tâches et leur stockage

Comme demandé dans le sujet, les tâches sont stockées en dur sur le disque dans différents fichiers. Elles se trouvent dans le répertoire `saturnd_dir/tasks`, ce qui fait qu'elles sont présentes de manière durable.  
Chaque tâche créée possède son propre répertoire ayant comme nom son taskid. Dans chaque répertoire de task, on trouve les fichiers suivants : 
-`argv` : qui contient la commande à effectuer et ses arguments
-`time_exec` : qui contient le temps d'exécution de la commande. C'est ce fichier qui va être ouvert et lu toutes les minutes, afin de vérifier s'il la commande doit être exécutée. 
-`exit` : qui contient la liste de tous les temps d'exécution ainsi que les codes de sortie de la tâche
Ces trois fichier sont créés lors de la création de la tâche. 
-`stout` : qui contient la sortie standard de la dernière exécution de la tâche 
-`stderr` : qui contient la sortie erreur de la dernière exécution de la tâche

En plus du dossier `tasks/`, on trouve à la racine de `saturnd_dir/` le fichier `taskid_max` qui permet de savoir quelle est le taskid de la dernière tâche créée, et donc de savoir où l'on s'est arrêté dans la création de tâches.
Le répertoire `saturnd_dir` et le fichier `taskid_max` sont créés au lancement de saturnd s'ils ne sont pas déjà présents. 
Un [diagramme représentant l'arborescence du stockage des tâches](https://gaufre.informatique.univ-paris-diderot.fr/jeanmari/sy5-projet-2021-2022/blob/master/Diagramme_Saturnd.pdf) se trouve à la racine du dépot.



## IV- Descriptions de nos requêtes

### A - Commandes de bases d'un exécuteur de tâches

#### `CREATE`

Le code de cette requête se trouve dans le fichier [`src/cmd_create.c`](https://gaufre.informatique.univ-paris-diderot.fr/jeanmari/sy5-projet-2021-2022/blob/master/src/cmd_create.c).
En recevant la requête `-c [TIME] [CMD] [ARGS]` de la part de Cassini, le démon va créer une nouvelle tâche.
On va d'abord récupérer la valeur de la dernière taskid créée dans le fichier `saturnd_dir/taskmax`. Si ce dernier n'existe pas (c'est-à-dire que c'est la première tâche que l'on crée), le fichier sera créé. Sinon, on ouvre ce fichier en `O_RDWR | O_TRUNC` afin d'incrémenter la `taskid_max`.
On va ensuite créer le répertoire et les fichiers de cette tâche (argv, time_exec et exit). On les initialise alors. 
L'initialisation de `time_exec` se fait via le buffer `temps`, dans lequel on va récupérer l'information transmise par la `fifo_request`. Ce buffer va être écrit dans le fichier `time_exec`, après la conversion des champs en `htobe()`. On va procéder de manière analogue pour l'écriture de la commande et de ses arguments dans le fichier `argv`. La difficulté supplémentaire ici étant de récupérer argument par argument la totalité de la commande, le champ `taille` permet de lire et d'écrire les arguments de bonne taille.
Enfin, on envoie comme réponse à cassini un buffer contenant `OK` suivi de la taskid de la tâche créée. 

#### `REMOVE`

Le code de cette requête se trouve dans le fichier [`src/cmd_remove.c`](https://gaufre.informatique.univ-paris-diderot.fr/jeanmari/sy5-projet-2021-2022/blob/master/src/cmd_remove.c).
Quand cassini envoie la requête `-r` suivie d'un taskid, saturnd va lire la taskid et va supprimer un à un chacun des fichiers du répertoire `taskid/`. Puis, on va supprimer le répertoire `taskid/`. Enfin, on initialise un buffer contenant la réponse `OK` que l'on envoie à Saturnd.  

#### `TERMINATE`

Le code de cette requête se trouve dans le fichier [`src/saturnd.c`](https://gaufre.informatique.univ-paris-diderot.fr/jeanmari/sy5-projet-2021-2022/blob/master/src/saturnd.c)
A la réception d'une requête terminate `-q`, le père envoie un signal `SIGTERM` à son fils Le fils va quand à lui attendre la fin de tous ses enfants avant de lui même quitter. Enfin, le père répond à cassini `OK`, ferme et quitte. 
Le démon finit donc de manière propre.


### B - Commandes "à réponse longue".
	
Ces commandes ont des tailles de réponses variables, et peuvent donc être potentiellement dangereuses pour le système car on risque des overflows du FIFO de réponse, ce qui n'est pas bien détectable par le système et dévastateur pour son intégrité (bien qu'en realité, la taille d'une FIFO sur linux soit de 65Kb ce qui rend difficile l'overflow avec la commande LIST et TIME_EXIT_CODE mais qui reste réel pour les commandes STD_OUT et STD_ERR).
Nous avons donc pris le parti d'écrire une constante `BUFFER_BLOCK` dans [`utils.h`](https://gaufre.informatique.univ-paris-diderot.fr/jeanmari/sy5-projet-2021-2022/blob/master/include/utils.h) (égale à 512) et d'écrire en block de BUFFER_BLOCK dans le pipe.
Cela va en contradiction avec l'idée que l'on écrit nos informations d'un coup dans le pipe afin d'éviter les blocages ou les erreures de lecture. 
Cependant nous avons fait attention à que Cassini lise avec des `read` non bloquant (`NO_HANG`) qu'on lance tant que toute l'information que l'on cherche à lire ne soit pas lue dans sa totalité.


#### `LIST`

Lorsque Cassini envoie la commande `-l` à Saturnd, ce dernier va créer un buffer qui va contenir la totalité des informations cherchées pour toutes les tasks (`buf_all_tasks`). 
Il va aussi instancier un unit32 "nbtasks" qui va lui permettre de compter le nombre de tasks lue en s'incrémentant à chaque itération du while ci dessous.
	
Il va alors ouvrir le répertoire `tasks/` et va parcourir les tâches en attribuant dans une structure `dirent` tous les répertoires de tâches lus.
	
Pour chacune des tâches, il va instancier un buffer contenant la totalité des informations relatives à la task envoyée à Cassini (`buff_inter`) et récuperer le nom de la structure ce qui va lui permettre de recuperé la taskid et de créer les chemins vers les fichiers `argv` et `time_exec` de la tâche.
	
Il va alors ouvrir le fichier time_exec de la tâche et lire et écrire dans "buf_inter" la structure timing de la tâche.
Ensuite il va instancier un buffer qui servira pour contenir la commandeline (buf_cmd) ouvrir la fichier "argv" et lire la commandline en lisant d'abord le nombre d'argument, puis pour chaque argument que lui indique ce nombre va récuperer le nombre d'octets de la chaîne de caractères et puis les caractères de la chaîne.

Par la suite, le `buf_cmd` sera copié dans le `buf_inter` (en faisant un `realloc` si la taille de `buff_inte`r ne permet de contenir toute la `commandeline`) et
`buf_inter` sera copié dans `buf_all_tasks` (en faisant un `realloc` si la taille de `buf_all_tasks` ne permet de contenir tout `buf_inter`).

Puis à la fin de la boucle, un buffer qui va contenir la réponse global de Cassini va être instancié (`buff_global`) auquel on copiera `OK` en uint16_t, le `nbtasks` et `buff_inter`. Puis on écrira le buffer dans le pipe en block comme indiqué en introduction de la partie.

Si une erreur se produit lors de la lecture de la tâche, on continue la boucle en abandonnant la lecture de cette tâche, et en décrementant nbtasks, cette tâche sera alors omise à Cassini lors de la réponse. 
Le code décris est présent dans le fichier [`src/cmd_list.c`](https://gaufre.informatique.univ-paris-diderot.fr/jeanmari/sy5-projet-2021-2022/blob/master/src/cmd_list.c).

#### `TIME_EXIT_CODE`
Lorsque Cassini envoie la commande `-x` suivit d'une taskid, saturnd va alors tenter d'ouvrir le répértoire portant le nom de cette tâche.
	
Si le répertoire ne s'ouvre pas alors (`opendir==-1`), c'est que la tâche n'existe pas. Saturnd renvoie un code d'erreur.
	
Si le répertoire s'ouvre, alors on va ouvrir le fichier `exit` contenu dans le répertoire ouvert et initialisé un buffer (appelé `buffer`) qui va contenir l'information lue par le fichier.
On va ensuite lire par block de `TIME_EXIT+EXITCODE` notre fichier en faisant des conversions `htobe()` à nos champs de `timing` (nécéssaire pour un bon fonctionnement de l'affichage par Cassini).
Ces blocks vont alors être copiés dans `buffer` jusqu'à ce que la totalité du fichier soit lu (le `read()` renvoie 0) et pour chaque block lu, on incrémentera un `uint32_t` (taille).
Ce buffer possède initialement la taille de 10 block, lorsqu'on risque un overflow, un `realloc` rajoute une taille supplémentaire de 10 block.
Puis, on va initialiser un buffer qui va contenir la réponse envoyée à Cassini dans lequel on va écrire `OK` en uint16_t, taille puis "buffer" (la taille du `malloc` est connue d'avance grâce à `taille`, nous évitant les `realloc`).
On écrira ensuite dans le pipe en block comme indiqué en introduction de la partie.
Si la tâche n'a jamais été executée, le fichier sera vide, on renvera donc juste OK suivit de 0 à Cassini.

#### `STD (STDOUT et STDERR)`
Rien ne diffère entre les deux commandes à l'exception du fichier ouvert, stderr pour `STDERR (-e)` et `stdout pour STDOUT (-o)`.
Lorsque Cassini envoie la commande suivie d'une taskid, saturnd va alors essayer d'ouvrir le répertoire et le fichier `argv` contenu dans ce dernier de la tâche correspondante.
-Si le répertoire ne s'ouvre pas, alors (`opendir==-1`) et saturnd renvoie un code error à Cassini, car la tâche n'existe pas.
-Si le répertoire s'ouvre alors, on va ouvrir le fichier `argv` contenu dans le répertoire ouvert et initialiser un buffer (appelé `buffer`) qui va contenir l'information lue par le fichier.
Si le fichier `sdtout` ou `stderr` ne s'ouvre pas (`open==-1`), alors la tâche n'a pas encore été créée car ses fichiers sont créés lors de l'exécution par le processus exécuteur.
On va alors renvoyer à Cassini l'erreur avec un code d'erreur indiquant que la tâche n'a jamais été executée.
Si le fichier s'ouvre on va alors initialiser un buffer puis essayer de le remplir en lisant le fichier et en tenant compte de la taille lue (qui sera utile à Cassini et à l'allocation de la taille du buffer final). 
Tant que la valeur retour du `read()` est égale à la taille du buffer, on va realloc du buffer de manière linéaire et continuer de read.
Puis à la fin de la boucle, un buffer qui va contenir la réponse global de cassini va être instancié (`buff_global`) auquel on copiera `OK` en `uint16_t`,la taille et le buffer de lecture. Puis on écrira ce buffer dans le pipe en block comme indiqué en introduction de la partie.
On ouvre en plus le fichier `argv` car si une commande à été supprimée mais est en train de se faire exécutée, les fichiers sont encore gardés non supprimés le temps que l'exécution se fasse (en le transformant en `.nfs`) ce qui a pour effet de les maintenir en vie le temps de l'execution le répertoire. On sait que `argv` est lu un temps infime durant l'exécution, il est donc supprimé lors du `remove`, ce test permet donc de voir si la tâche existe toujours, ou il s'agit d'une tâche "zombie" afin de renvoyer un message d'erreur à Cassini le cas échéant.

### C - Commandes additionnelles

La modularité du code va nous permettre de réutiliser des fonctions écrites pour d'autres requêtes additionnelles que nous avons choisi d'ajouter afin de rendre notre exécuteur de tâches plus complet.
Pour ajouter ces tasks, il a fallu déclarer de nouvelles requêtes dans [`include/client-request.h`](https://gaufre.informatique.univ-paris-diderot.fr/jeanmari/sy5-projet-2021-2022/blob/master/include/client-request.h)ainsi que de nouvelles erreurs dans [`include/server-reply.h`](https://gaufre.informatique.univ-paris-diderot.fr/jeanmari/sy5-projet-2021-2022/blob/master/include/server-reply.h).
 

#### `CLIENT_REQUEST_SET_CMD `

Cette commande va permettre de changer la tâche et ses arguments de taskid taskid passé dans la requête. 
On appelle cette commande avec l'argument `-z` suivi du taskid de la tâche dont on veut modifier les arguments et de la nouvelle commande. 
On vérifie d'abord si la commande de taskid donné existe. Si ce n'est pas le cas, on initialise un buffer contenant l'erreur appropriée que l'on renvoie à cassini. 
Si la tâche existe bien, on ouvre le fichier `argv`, que l'on tronque pour y écrire la nouvelle nouvelle commande envoyée par cassini. De manière analogue à `CREATE`, de manière séquentielle, on lit d'abord la taille de la chaine à écrire, puis on écrit dans le fichier l'argument de cette taille. Enfin, on initialise un buffer contenant `OK` pour Cassini. 

#### `EXEC_TASK`
Cette commande va permettre d'exécuter une tâche immédiatement. On appelle cette commande avec l'argument `-i` suivi du taskid souhaité. Si le répertoire de nom `taskid` n'existe pas, on renvoie une erreur à Cassini dans un buffer. Sinon, on va réutiliser les fonctions d'exécution de taĉhes contenues dans [`src/saturnd_child.c`](url) et on va mettre à jour les fichiers `stdout`, `stderr` et `exit`. Saturnd renvoie `OK` à Cassini.  

#### `DELETE_ALL `
Cette commande va permettre de supprimer toutes les tâches créées. On l'appelle avec l'argument `-s`. 
On va appeler sur chaque répertoire de tâches contenu dans `tasks/` la fonction qui gère leur suppression (dans [`src/cmd_remove.c`](https://gaufre.informatique.univ-paris-diderot.fr/jeanmari/sy5-projet-2021-2022/blob/master/src/cmd_remove.c). 
Saturnd renvoie à Cassini `OK` suivi du nombre de tâches effectivement supprimées. 
En utilisant cette commande, puis l'option `RESET`, on peut donc effacer toutes nos tâches et repartir d'une taskid 1. 


#### `SW_TIME `
Le code de cette commande se trouve dans [`src/cmd_switch_time.c`](https://gaufre.informatique.univ-paris-diderot.fr/jeanmari/sy5-projet-2021-2022/blob/master/src/cmd_switch_time.c). 
Cette commande va permettre de modifier le temps d'exécution d'une tâche déjà créée. On l'appelle avec l'argument `-t` suivi du taskid de la tâche en question et du nouveau temps d'exécution souhaité. Saturnd récupère son taskid, et on tente d'ouvrir son répertoire pour voir si la tâche existe. Si ce n'est pas le cas, on initialise un buffer dans lequel on écrit un code erreur que l'on renvoie à Cassini. Si le répertoire existe, on ouvre le fichier time_exec et on remplace l'ancien temps d'exécution par le nouveau. Saturnd renvoie `OK` dans un buffer à Cassini. 

#### `RESET_TASK_MAX` 
On appelle cette commande avec l'argument `-w`. 
Après la supression de plusieurs tâches, on peut se retrouver dans la situation suivante : la prochaine tâche créée aura un taskid plus grand que la première taskid effectivement disponible. 
On va donc parcourir le répertoire de tâches `tasks/` à la recherche de la tâche présentant le plus grand taskid. Il suffit de remplacer la valeur dans `taskid_max` par la valeur trouvée. On initialise un buffer qui contient OK suivi de la nouvelle `taskid_max` trouvée. 
S'il n'y a pas encore de tâches créées, et donc par conséquent pas de fichier `taskid_max`, Saturnd renvoie une nouvelle erreur (`SERVER_REPLY_NO_TASK_ERROR)` à Cassini. 


## V-Potentiels problèmes de notre exécuteur de tâches

Notre système de processus père-fils de notre exécuteur de tache possède un problème potentiel, la synchronisation.

En effet il est possible qu'une tâche soit éditée, ou supprimée alors que un processus est en train de l'exécuter, ou pire en train de lire des instructions dans les fichiers de la tâche pour l'executer.

Cependant nous avons décidé de continuer avec ce système car après avoir listé les erreurs de synchronisation possible, nous en avons conclu qu'elle n'était pas génant au bon fonctionnement de l'exécuteur.
Voici la liste des erreurs de synchronisation possible en fonction de la commande de Cassini:


Si un `CREATE` est réalisé à un temps donné d'une tâche qui doit être executé à ce temps précis:
Dans ce cas là, il y a de grande chance que la tasks de soit pas executée par saturnd à la minute où elle est créée.

Si un `GET_STDOUT/STDERR` est réalisé à un temps donné, d'une tâche qui est en train d'être exécutée.

Dans ce cas, le` GETSTDOUT/STDERR` peut, soit donné la valeur de la dernière execution avant cette minute, soit return la nouvelle valeur après exécution à ce "temps donné".

Un cas uniforme se produit lorsque on demande les TIMEEXITCODE d'une tache ou on pourra avoir le nouveau `time_exit_code` si la tâche a eu le temps d'être executé avant la lecture de tache ou non.

Si un `REMOVE` est réalisé quand une tâche est executée.

Dans ce cas, l'exécution va tout simplement échoué ou alors écrire sa sortie dans le fichier `STDOUT/STDERR` devenu des `.nfs` qui seront supprimé après l'exec, le file contenant les exitcode sera aussi supprimé après qu'on ait copié la valeur de l'`exec()` dedans

A noté que même si un `remove()` laisse un taske directory avec des `.nfs` dedans, il n'est pas pris en compte lors des `LIST`.

Si un `EXIT` est realisé alors que certaines tâches sont encore en train d'être executées,

Dans ce cas, le processus 1 va simplement adopté notre processus attendant l'exit code de l'`exec()`, et lorsque il aura fini, le processu 1 va le `wait()`, l'empêchant de rester en zombie.

A noter que si le système est fermé, alors que certaines executions ne sont pas finies, elles seront alors interompues, ce qui est plutôt logique.

Nous avons ensuite nos nouvelles tâches qui peuvent elles aussi créer de la compétition entre les process.

Notre commande `REMOVE_ALL` peut avoir les mêmes problèmes de synchronisation que `REMOVE`.

Si nos commandes modifiant les propriétés d'une task sont lancées à la minute où la tâche doit être executée.
Dans ce cas on a trois cas possibles:
- Si l'exec() à cette minute ne s'était pas encore fait les modifications seront prise en compte
- Si l'exec() à cette minute a déjà était fais alors les modifications ne seront pas prise en compte pour cette minute
- Si l'exec() est en train de se faire et que les informations dans les files sont en train d'être recuperées en même temps quelles sont réécrites.
Dans le meilleur des cas on créer des données chimères illisibles et l'exécution échoue.
Dans le pire des cas, on exécute la commande line chimère qui peut soit échouer, soit écrire et exécuter une tâche fausse.


En conclusion, les erreurs de synchronisation sont inexistantes tant que on ne lance pas des commandes sur une taskid à la minute où elle devrait se faire exécuter.

Nous avons juger cette contrainte plutôt minimes et quand bien même elle n'est pas respecté, les erreurs obtenues sont minimes et affectent juste l'exécution à cette minute précise.

Il faut juste être conscient de cette subtilité, c'est pour ça qu'elle a été indiquée dans le `help` de cassini.


## VI-Conclusions


Voici pour finir une liste d'extensions que nous aurions aimé implémenter au projet mais que nous n'avons pas fait par manque de temps : 

- Une commande pour reset le fichier contenant les `exit_code` et leurs temps.

- Une commande pour cloner une tache, créant donc une nouvelle tâche avec les mêmes propriétés.

- Un fichier `error_log` contenant à chaque ligne l'historique des potentielles erreurs survenues lors de l'exécution d'une tâche et d'une commande avec le temps à laquelle l'erreur est survenue en plus et une commande cassini pour le lire.

- Un système pour éviter les erreurs de synchronisation entrainant l'exécution de commande chimère.


En conclusion, merci d'avoir prété attention à notre projet et à notre compte-rendu.

Nous avons compter avoir coder plus de 35 heures chacun afin de vous proposer ce rendu, nous sommes tous d'accord sur le fait que le projet à été réalisé grâce à des quantités de travail équitable tout au long du projet.


En espérant que vos tests se passent bien,


Emma Soufir

Roude Jean-Marie

Rémi Poulard

