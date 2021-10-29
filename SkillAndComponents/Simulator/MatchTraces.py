import random
import itertools

def RemoveEmptyLines (str):
    lines = str.split("\n")
    non_empty_lines = [line for line in lines if line.strip() != ""]

    string_without_empty_lines = ""
    for line in non_empty_lines:
        string_without_empty_lines += line + "\n"

    return string_without_empty_lines



def ComputeDiscrepances(output_list,output_list_trace,deleted_trace):
    
    index = []
    index_to_remove = []
    for i in range (0, len(output_list)) :
    
        if deleted_trace == False :
            new_output_list_trace = list(output_list_trace.split("\n"))
            if new_output_list_trace [i] != output_list[i] :
                    index.append(i)

    for i in range(0,len(index)-1) :
        if new_output_list_trace[int(index[i])] == output_list[int(index[i+1])] :
            index_to_remove.append(index[i])
            
        if new_output_list_trace[int(index[i+1])] == output_list[int(index[i])]:
            index_to_remove.append(index[i+1])
            

    for j in range (0,len(index_to_remove)) :
        index.remove(index_to_remove[j])
    
    disc_trace = len(index)

    return disc_trace

class Mealy(object):
    """Mealy Machine : Finite Automata with Output """

    def __init__(self, states, input_alphabet, output_alphabet, transitions, initial_state):
        """
        6 tuple (Q, ∑, O, δ, X, q0) where −
        states is a finite set of states.
        alphabet is a finite set of symbols called the input alphabet.
        output_alphabet is a finite set of symbols called the output alphabet.
        transitions is the resultant data dictionary of input and output transition functions
        initial_state is the initial state from where any input is processed (q0 ∈ Q).
        """
        self.states = states
        self.input_alphabet = input_alphabet
        self.output_alphabet = output_alphabet
        self.transitions = transitions
        self.initial_state = initial_state
    def get_output_from_string_single(self, string):
            """Return Mealy Machine's output when a given string is given as input"""

            #temp_list = list(string)
            current_state = self.initial_state
            output = ''
            #for x in temp_list:
            output+= self.transitions[current_state][string][1]
            current_state = self.transitions[current_state][string][0]

            return output

    def get_output_from_string(self, list_string):
        """Return Mealy Machine's output when a given string is given as input"""

        #temp_list = list(lista)
        current_state = self.initial_state
        output = ''
        for i in range (0,len(list_string)):
            output+= self.transitions[current_state][list_string[i]][1]+"\n"
            current_state = self.transitions[current_state][list_string[i]][0]

        return output

    def __str__(self):
        output = "\nMealy Machine" + \
                 "\nStates " + str(self.states) + \
                 "\nTransitions " + str(self.transitions) + \
                 "\nInital State " + str(self.initial_state) + \
                 "\nInital Alphabet " + str(self.input_alphabet) + \
                 "\nOutput Alphabet" + str(self.output_alphabet)

        return output


def convert_fromCh_toSt(s):
  
    # initialization of string to ""
    new = ""
  
    # traverse in the string 
    for x in s:
        new += x 
  
    # return string 
    return new

f = open('Mealy_A.txt', 'r')
x = f.read()

mealyA = eval(x)


f = open('Mealy_B.txt', 'r')
x = f.read()

mealyB = eval(x)

f = open('Mealy_C.txt', 'r')
x = f.read()

mealyC = eval(x)


f = open('Input.txt', 'r')
x = f.read()
input_list = list(x.split("\n"))

f = open('Ouput.txt', 'r')
x = f.read()
output_list = list(x.split("\n"))

if input_list[len(input_list)-1] == '':
    input_list = input_list[:-1]

#output_listA = mealyA.get_output_from_string(input_list) 

#output_listB = mealyB.get_output_from_string(input_list)

discA = 0
discB = 0
discC = 0

deletedA = False
deletedB = False
deletedC = False

remaining = []
output_listA =''
output_listB =''
output_listC =''



if deletedA == False :
    try :
        output_listA = mealyA.get_output_from_string(input_list)
    except KeyError :
        deletedA = True
        print('DELETED Trace A ')

        try :
            output_listB = mealyB.get_output_from_string(input_list)
        except KeyError :
            #remaining.append("trace C")
            deletedB = True
            print('DELETED Trace B ')
        try :
            output_listC = mealyC.get_output_from_string(input_list)
        except KeyError :
            #remaining.append("trace B")
            deletedC = True
            print('DELETED Trace C ')



if deletedB == False :
    try :
        output_listB = mealyB.get_output_from_string(input_list)
    except KeyError :
        print('DELETED Trace B')
        deletedB = True

        try :
            output_listA = mealyA.get_output_from_string(input_list)
        except KeyError :
            deletedA = True
            print('DELETED Trace A ')

        try :
            output_listC = mealyC.get_output_from_string(input_list)
        except KeyError :
            deletedC = True
            print('DELETED Trace C ')


if deletedC == False :
    try :
        output_listC = mealyC.get_output_from_string(input_list)
    except KeyError :
        print('DELETED Trace C ')
        deletedC = True
        try :
            output_listA = mealyA.get_output_from_string(input_list)
        except KeyError :
            deletedA = True
            print('DELETED Trace A ')

        try :
            output_listB = mealyB.get_output_from_string(input_list)
        except KeyError :
            deletedB = True
            print('DELETED Trace B ')



discA = ComputeDiscrepances(output_list,output_listA,deletedA)
discB = ComputeDiscrepances(output_list,output_listB,deletedB)
discC = ComputeDiscrepances(output_list,output_listC,deletedC)



print ("disc A :",discA)
print ("disc B :",discB)
print ("disc C :",discC)

#print("deleted A ", deletedA)
#print("deleted B ", deletedB)
#print("deleted C ", deletedC)

if deletedA :
    if deletedB and deletedC == False and discC < 5:
        remaining.append("trace C") 

    elif deletedC and deletedB == False and discB < 5:
        remaining.append("trace B") 

    elif  deletedB == False and deletedC == False:
        if discB > discC :
                remaining.append("trace C")
        elif discC > discB :
                remaining.append("trace B")

if deletedB :
    if deletedA and deletedC == False and discC < 5:
        remaining.append("trace C")

    elif deletedC and deletedA == False and discA < 5 :
        remaining.append("trace A")

    elif deletedA == False and deletedC == False :

        if discA > discC :
                remaining.append("trace C")
        elif discC > discA :
                remaining.append("trace A")

if deletedC :
    if deletedA and deletedB == False and discB < 5:
        remaining.append("trace B")

    elif deletedB and deletedA == False and discA < 5 :
        remaining.append("trace A")

    elif deletedA == False and deletedB == False :

        if discB > discA :
                remaining.append("trace A")
        elif discA > discB :
                remaining.append("trace B")

if deletedA == False and deletedB == False and deletedC == False :
    if discA < discB and discA < discC :
        remaining.append("trace A")
    elif discB < discA and discB < discC :
        remaining.append("trace B")
    elif discC < discA and discC < discB :
        remaining.append("trace C")


remaining = list(dict.fromkeys(remaining))

if len(remaining) == 1 :
    print(remaining[0])

elif len(remaining) == 0 :
    print('Trace not found')


