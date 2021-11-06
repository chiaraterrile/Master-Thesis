import re 
import itertools
from graphviz import Source

def RemoveEmptyLines (str):
    lines = str.split("\n")
    non_empty_lines = [line for line in lines if line.strip() != ""]

    string_without_empty_lines = ""
    for line in non_empty_lines:
        string_without_empty_lines += line + "\n"

    return string_without_empty_lines
def Convert(string):
    li = list(string.split("\n"))
    return li


path = 'graph_simulator_v2_C'
s = Source.from_file(path)
#with open("Transitions.txt", "w") as text_file:
 #   text_file.write(s.source)


x = s.source

x = x.replace("digraph g {", " ")
x = x.replace("\t", "")

x = RemoveEmptyLines(x)

# ------------ states lines ----------------
states ='\n'
n_states = 0
for line in x.splitlines():

    if ' [shape="circle"' in line:
        states = states+ line + '\n'
        n_states = n_states +1 

new_states ='\n'
for line in states.splitlines() :
    states = line.partition(' [shape="circle" label=')
    states = states[0]
    new_states = new_states+ states + '\n'
    
new_states = RemoveEmptyLines(new_states)
new_states = Convert(new_states)
mealy = 'Mealy(['
for i in range(0,n_states) :
    mealy = mealy +"'"+ new_states[i]+"'"+ ','
    x = x.replace("s"+str(i)+' [shape="circle" label="'+str(i)+'"];','')

mealy = mealy[:-1] +"],\n["

# ---------------- input and output lines -------------------
x = RemoveEmptyLines(x)
input_lines ='\n'
output_lines ='\n'
InOu ='\n'

for line in x.splitlines():

    if '/' in line:
        InOu = InOu+ line + '\n'
        

for line in InOu.splitlines(): 
    input_lines = input_lines + line[line.find('"')+len('"'):line.rfind(' /')] +'\n'
    output_lines =  output_lines + line[line.find('/ ')+len('/ '):line.rfind('"')] +'\n'

input_lines = RemoveEmptyLines(input_lines)
input_lines = Convert(input_lines)
input_lines = input_lines[:-1]
new_input_lines = list(dict.fromkeys(input_lines))
output_lines = RemoveEmptyLines(output_lines)
output_lines = Convert(output_lines)
output_lines = output_lines[:-1]
new_output_lines = list(dict.fromkeys(output_lines))

for i in range(0,len(new_input_lines)) :
    mealy = mealy +"'"+ new_input_lines[i]+"'"+ ','

mealy = mealy[:-1] +"],\n["

for i in range(0,len(new_output_lines)) :
    mealy = mealy +"'"+ new_output_lines[i]+"'"+ ','

mealy = mealy[:-1] +"],\n{\n\t"

# ------------- transitions lines -------------------
x = '\n'+x
prec_state ='\n'
next_state ='\n'
for line in x.splitlines() :
    prec_state = prec_state + line[line.find('\n')+len('\n'):line.rfind(' ->')] +'\n'
    next_state =  next_state + line[line.find('-> ')+len('-> '):line.rfind(' [')] +'\n'

prec_state = RemoveEmptyLines(prec_state)
prec_state = Convert(prec_state)
prec_state = prec_state[:-3]

next_state = RemoveEmptyLines(next_state)
next_state = Convert(next_state)
start_state = next_state[len(next_state)-2]
next_state = next_state[:-3]


i = 0
while i < (len(prec_state)-1) :
    if prec_state[i]!= prec_state[i+1] :
        mealy = mealy + "'" + prec_state[i]+ "' : {\n\t\t'"+input_lines[i]+"' : ('"+next_state[i]+"','"+output_lines[i]+"')\n\t},\n\t"
        i += 1
    elif prec_state[i]== prec_state[i+1] :
        mealy = mealy + "'" + prec_state[i]+ "' : {\n\t\t'"+input_lines[i]+"' : ('"+next_state[i]+"','"+output_lines[i]+"'),"
        i = i+1
        while prec_state[i]== prec_state[i-1] and i < (len(prec_state)-1) :
            mealy = mealy+ "\n\t\t'"+input_lines[i]+"':('"+next_state[i]+"','"+output_lines[i]+"'),"
            i += 1
        if prec_state[i] == prec_state[i-1] :
            mealy = mealy+ "\n\t\t'"+input_lines[i]+"':('"+next_state[i]+"','"+output_lines[i]+"')"
        mealy = mealy +'\n\t\t},\n\t'

if prec_state[i] != prec_state[i-1] :
            mealy = mealy + "'" + prec_state[i]+ "' : {\n\t\t'"+input_lines[i]+"' : ('"+next_state[i]+"','"+output_lines[i]+"')\n\t},\n\t"
       
       
mealy = mealy[:-3] +"\n},\n'"+start_state+"')"

# substitute state names with numbers 

for i in range (0,n_states) :
    mealy = mealy.replace('s'+str(i),str(i))


with open("Mealy_C.txt", "w") as text_file:
    text_file.write(mealy)
