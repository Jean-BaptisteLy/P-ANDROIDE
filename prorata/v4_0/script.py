import os

consensus_a = 0.0
consensus_b = 0.0
consensus_c = 0.0
consensus_d = 0.0

nbre_runs = 10

for i in range(nbre_runs):
	os.system('make && ./dissemination_exploration')

	theorique_a = False
	theorique_b = False
	theorique_c = False
	theorique_d = False

	experimental_a = False
	experimental_b = False
	experimental_c = False
	experimental_d = False

	with open("./resultats.txt","r") as file:
		for line in file:
			if (line=="nbre_theorique_a\n"):
				theorique_a = True
			if (line=="nbre_theorique_b\n"):
				theorique_b = True
			if (line=="nbre_theorique_c\n"):
				theorique_c = True
			if (line=="nbre_theorique_d\n"):
				theorique_d = True
			if (theorique_a == True and line != "nbre_theorique_a\n"):
				nbre_theorique_a = float(line)
				theorique_a = False
			if (theorique_b == True and line != "nbre_theorique_b\n"):
				nbre_theorique_b = float(line)
				theorique_b = False
			if (theorique_c == True and line != "nbre_theorique_c\n"):
				nbre_theorique_c = float(line)
				theorique_c = False
			if (theorique_d == True and line != "nbre_theorique_d\n"):
				nbre_theorique_d = float(line)
				theorique_d = False

			if (line=="nbre_experimental_a\n"):
				experimental_a = True
			if (line=="nbre_experimental_b\n"):
				experimental_b = True
			if (line=="nbre_experimental_c\n"):
				experimental_c = True
			if (line=="nbre_experimental_d\n"):
				experimental_d = True
			if (experimental_a == True and line != "nbre_experimental_a\n"):
				nbre_experimental_a = float(line)
				experimental_a = False
			if (experimental_b == True and line != "nbre_experimental_b\n"):
				nbre_experimental_b = float(line)
				experimental_b = False
			if (experimental_c == True and line != "nbre_experimental_c\n"):
				nbre_experimental_c = float(line)
				experimental_c = False
			if (experimental_d == True and line != "nbre_experimental_d\n"):
				nbre_experimental_d = float(line)
				experimental_d = False

	consensus_a += nbre_experimental_a
	consensus_b += nbre_experimental_b
	consensus_c += nbre_experimental_c
	consensus_d += nbre_experimental_d

consensus_a = consensus_a / nbre_runs
consensus_b = consensus_b / nbre_runs
consensus_c = consensus_c / nbre_runs
consensus_d = consensus_d / nbre_runs

print("consensus_a moyen =",consensus_a)
print("consensus_b moyen =",consensus_b)
print("consensus_c moyen =",consensus_c)
print("consensus_d moyen =",consensus_d)

'''
print(nbre_theorique_a)
print(nbre_theorique_b)
print(nbre_theorique_c)
print(nbre_theorique_d)

print(nbre_experimental_a)
print(nbre_experimental_b)
print(nbre_experimental_c)
print(nbre_experimental_d)
'''
