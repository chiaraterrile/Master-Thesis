
f = open('AbstractTraces.txt', 'r')
x = f.read()


IOstring = x.replace('input','i/OK')
IOstring = IOstring.replace('output','Delta/o')

#with open("IO_traces.txt", "w") as text_file:
   # text_file.write(IOstring)

#f = open('IO_traces.txt', 'r')
#content = f.read()
IOstring = IOstring.replace("\n", "/")

A = IOstring.split('/')


#input_txt ='"' + A[0] +'"'
#output_txt ='"'+ A[1] +'"'

input_txt = A[0] + '\n'
output_txt = A[1] + '\n'
#input_prova = A[0] + '\n'
for i in range(1,int(len(A)/2)):
    
    #input_txt = input_txt+',"'+A[2*i]+'"'
    input_txt = input_txt+A[2*i]+'\n'
   
   
for i in range(1,int(len(A)/2)):
    #output_txt = output_txt+',"' + A[2*i+1] +'"'
    output_txt = output_txt+ A[2*i+1] +'\n'

with open("Input.txt", "w") as text_file:
    text_file.write(input_txt)



with open("Ouput.txt", "w") as text_file:
    text_file.write(output_txt)

