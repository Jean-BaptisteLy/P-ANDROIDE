import os
import matplotlib.pyplot as plt

nbre_runs = 10
nombre_intervalle_temps_min = 17 # à modifier de même dans le fichier .c sinon problème
intervalle_temps_min = 10 # à modifier de même dans le fichier .c sinon problème

liste_qualites_a = [0.0 for i in range(nombre_intervalle_temps_min)]
liste_qualites_b = [0.0 for i in range(nombre_intervalle_temps_min)]
liste_qualites_global = [0.0 for i in range(nombre_intervalle_temps_min)]

consensus_a = 0.0
consensus_b = 0.0

seuils = [25,40,60]
listes_qualites_seuils = []


for s in seuils:
	print("Bien changer le fichier .c avec le seuil",s,"avant d'appuyer entrée")
	input()
	os.system('make')
	liste_qualites_b = [0.0 for i in range(nombre_intervalle_temps_min)]
	for i in range(nbre_runs):
		os.system('./dissemination_exploration')

		flag_a = False
		flag_b = False
		flag_global = False

		with open("./resultats_nets.txt","r") as file:
			for line in file:

				if (line=="qualite_consensus_a\n"):
					flag_a = True
				if (line=="qualite_consensus_b\n"):
					flag_b = True
				if (line=="qualite_consensus_global\n"):
					flag_global = True

				if (flag_a == True and line != "qualite_consensus_a\n"):
					qualite_consensus_a = line.split()
					flag_a = False
				if (flag_b == True and line != "qualite_consensus_b\n"):
					qualite_consensus_b = line.split()
					flag_b = False
				if (flag_global == True and line != "qualite_consensus_global\n"):
					qualite_consensus_global = line.split()
					flag_global = False

			for j in range(nombre_intervalle_temps_min):
				liste_qualites_a[j] += float(qualite_consensus_a[j])
				liste_qualites_b[j] += float(qualite_consensus_b[j])
				liste_qualites_global[j] += float(qualite_consensus_global[j])
	
		print("Run",i+1,":")
		#print("liste_qualites_a :",liste_qualites_a)
		print("liste_qualites_b :",liste_qualites_b)
		#print("liste_qualites_global :",liste_qualites_global)
	listes_qualites_seuils.append(liste_qualites_b)


def create_x(n,intervalle_temps_min):
    # n : nombre d'intervalles
    # intervalle_temps_min : nombre de min par intervalle
    x = []
    x.append(0)
    for i in range(1,n):
        x.append(intervalle_temps_min * i)
    return x

def graphe(x,yA,yB,yGlobal,seuils):
    titre = "Evolution du nombre d'agents pour la meilleure ressource sur "+str(nbre_runs)+" runs :"
    plt.title(titre)
    plt.xlabel("Temps en minutes")
    plt.ylabel("Qualité du consensus")
    
    #plt.plot(x, yA,c='red',label='Site A')
    for s in range(len(seuils)):
    	titre = "Seuil = "+str(seuils[s])
    	plt.plot(x, yB[s], label=titre)
    #plt.plot(x, yGlobal,c='purple', label='Consensus global')
    
    plt.grid()
    plt.legend()
    plt.savefig("temp.png")
    plt.show()

x = create_x(nombre_intervalle_temps_min,intervalle_temps_min)

yA = [i / float(nbre_runs) for i in liste_qualites_a]
yB = []
for s in listes_qualites_seuils:
	yB.append([i / float(nbre_runs) for i in s])
#yB = [i / float(nbre_runs) for i in liste_qualites_b]
yGlobal = [i / float(nbre_runs) for i in liste_qualites_global]

graphe(x,yA,yB,yGlobal,seuils)