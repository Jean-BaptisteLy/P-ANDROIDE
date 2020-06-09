# coding: utf-8
import json
import matplotlib.pyplot as plt

with open('./endstate.json') as json_data:
    data_dict = json.load(json_data)
    
nb_kilobots = len(data_dict["bot_states"])
nb_opinions = 0
for i in range(nb_kilobots):
    if(data_dict["bot_states"][i]["state"]["Opinion"] == 1):
        nb_opinions += 1
print("Opinions :",nb_opinions)
print("Kilo ticks :",data_dict["ticks"])

def create_x(n,step):
    # n : nombre d'intervalles
    # step : nombre de kiloticks
    x = []
    x.append(0)
    for i in range(1,n+1):
        x.append(step * i)
    return x

def graphe(x,y5,y15,y25,y60):    
    plt.title('Consensus en fonction du seuil')
    plt.xlabel('Temps en minutes')
    plt.ylabel('Consensus en pourcentage')
    '''
    plt.plot(x, y5,c='blue',label='Seuil à 5')
    plt.plot(x, y5,c='blue', label='Seuil à 5')
    plt.plot(x, y15,c='green', label='Seuil à 15')
    plt.plot(x, y25,c='red', label='Seuil à 25')
    plt.plot(x, y60,c='purple', label='Seuil à 60')
    '''
    plt.plot(x, y5)
    plt.legend()
    plt.show()

'''
x = create_x(12,10)
y5 = [50,50,60,50,58,69,71,82,97,100,100,100,100]
y5 = [i/100 for i in y5]
y15 = [50,44,55,81,97,100,100,100,100,100,100,100,100]
y15 = [i/100 for i in y15]
y25 = [50,71,91,97,98,99,100,100,100,100,100,100,100]
y25 = [i/100 for i in y25]
y60 = [50,48,49,49,63,75,76,76,85,91,90,92,100]
y60 = [i/100 for i in y60]
graphe(x,y5,y15,y25,y60)
'''