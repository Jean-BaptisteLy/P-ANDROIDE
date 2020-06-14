Lien de la vidéo de présentation : 

# P-ANDROIDE
Projet P-ANDROIDE de M1 : Recherche de consensus en robotique en essaim

Lien original : http://androide.lip6.fr/?q=node/553

On s'intéresse dans ce projet au problème du best-of-n en robotique essaim, dans lequel il s'agit pour un ensemble de robots aux capacités de communication et de calcul limitées. L'objectif de ce projet est d'étudier l'émergence de consensus en utilisant soit un algorithme dédié, soit un algorithme d'apprentissage. Le projet sera mené sur robots réels (une centaine de Kilobots), disponible à l'ISIR.

Dans un premier temps, il s'agit d'implémenter un algorithme existant permettant d'atteindre de manière distribuée un consensus entre deux ressources. Ces ressources sont représentés par des objets émettant un signal infra-rouge donnant leur qualité, et l'objectif de l'essaim est de choisir la ressource de plus grande qualité. Ces deux ressources sont placées aux deux extrémités d'une arène rectangulaire, et une source de lumière permet en fuyant ou poursuivant la lumière de facilement se diriger vers l'une ou l'autre des deux ressources.

Dans un second temps, on implémentera un algorithme d'apprentissage en ligne et distribué afin d'évaluer les difficultés que peuvent poser l'apprentissage de consensus (par opposition à l'utilisation d'un algorithme dédié). L'espace de recherche sera défini à partir des comportements de phototaxis, anti-phototaxis et déambulation libre -- il s'agira d'apprendre les conditions de transitions entre chaque comportement. On s'intéressera d'abord au cas ou l'essaim doit se regrouper autour de la ressource de plus grande valeur, puis le cas ou l'essaim doit distribuer ses forces au pro-rata de la valeur de chaque ressource (i.e. la distribution spatiale entre les deux ressources devra être à l'image de la valeur de chacune).

Selon le temps et les compétences des participants, l'amélioration du système de tracking visuel de robots pourra être repris et amélioré afin de permettre un suivi temps réel de l'essaim.

Bibliographie:
* Valentini et al. (2016) Collective decision with 100 Kilobots: Speed versus accuracy in binary discrimination problems. AAMAS.
* Valentini et al (2017) The best-of-n problem in robot swarms: Formalization, state of the art, and novel perspectives. Frontiers in AI and Robotics.
* Bredeche et al. (2012) Environment-driven distributed evolutionary adaptation in a population of autonomous robotic agents. MCMDS.
