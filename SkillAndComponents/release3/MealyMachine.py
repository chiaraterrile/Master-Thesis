import random
import itertools

def RemoveEmptyLines (str):
    lines = str.split("\n")
    non_empty_lines = [line for line in lines if line.strip() != ""]

    string_without_empty_lines = ""
    for line in non_empty_lines:
        string_without_empty_lines += line + "\n"

    return string_without_empty_lines

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


f = open('MealyB.txt', 'r')
x = f.read()

mealy = eval(x)

f = open('Input.txt', 'r')
x = f.read()
input_list = list(x.split("\n"))

if input_list[len(input_list)-1] == '':
    input_list = input_list[:-1]

output_list = mealy.get_output_from_string(input_list)

with open("output_machine.txt", "w") as text_file:
    text_file.write(output_list)
