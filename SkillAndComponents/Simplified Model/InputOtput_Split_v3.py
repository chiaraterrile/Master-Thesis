
f = open('AbstractTraces.txt', 'r')
x = f.read()


IOstring = x.replace('goTo_ki_input','goTo_ki_i/OK_goto_ki')
IOstring = IOstring.replace('level_input','level_i/OK_goto_ki')
IOstring = IOstring.replace('getStatus_ki_input','getStatus_ki_i/OK_getStatus_ki')
IOstring = IOstring.replace('halt_ki_input','halt_ki_i/OK_halt_ki')
IOstring = IOstring.replace('goTo_ch_input','goTo_ch_i/OK_goto_ch')
IOstring = IOstring.replace('getStatus_ch_input','getStatus_ch_i/OK_getStatus_ch')
IOstring = IOstring.replace('batteryStatus_input','batteryStatus_i/OK_batteryStatus')
IOstring = IOstring.replace('isAt_ki_input','isAt_ki_i/OK_isAt_ki')
IOstring = IOstring.replace('isAt_ch_input','isAt_ch_i/OK_isAt_ch')
IOstring = IOstring.replace('halt_ch_input','halt_ch_i/OK_halt_ch')



IOstring = IOstring.replace('output_goto_ki','Delta_goto_ki/o_goto_ki')
IOstring = IOstring.replace('output_getStatus_ki_run','Delta_getStatus_ki_run/o_getStatus_ki_run')
IOstring = IOstring.replace('output_getStatus_ki_suc','Delta_getStatus_ki_suc/o_getStatus_ki_suc')
IOstring = IOstring.replace('output_getStatus_ki_ab','Delta_getStatus_ki_ab/o_getStatus_ki_ab')
IOstring = IOstring.replace('output_halt_ki','Delta_halt_ki/o_halt_ki')
IOstring = IOstring.replace('output_goto_ch','Delta_goto_ch/o_goto_ch')
IOstring = IOstring.replace('output_getStatus_ch_run','Delta_getStatus_ch_run/o_getStatus_ch_run')
IOstring = IOstring.replace('output_getStatus_ch_suc','Delta_getStatus_ch_suc/o_getStatus_ch_suc')
IOstring = IOstring.replace('output_getStatus_ch_ab','Delta_getStatus_ch_ab/o_getStatus_ch_ab')
IOstring = IOstring.replace('output_batteryStatus_false','Delta_batteyStatus_false/o_batteryStatus_false')
IOstring = IOstring.replace('output_batteryStatus_true','Delta_batteyStatus_true/o_batteryStatus_true')
IOstring = IOstring.replace('output_isAt_ki_false','Delta_isAt_ki_false/o_isAt_ki_false')
IOstring = IOstring.replace('output_isAt_ki_true','Delta_isAt_ki_true/o_isAt_ki_true')
IOstring = IOstring.replace('output_isAt_ch_false','Delta_isAt_ch_false/o_isAt_ch_false')
IOstring = IOstring.replace('output_isAt_ch_true','Delta_isAt_ch_true/o_isAt_ch_true')
IOstring = IOstring.replace('output_halt_ch','Delta_halt_ch/o_halt_ch')
IOstring = IOstring.replace('output_level_high','Delta_level_high/o_level_high')
IOstring = IOstring.replace('output_level_medium','Delta_level_medium/o_level_medium')
IOstring = IOstring.replace('output_level_low','Delta_level_low/o_level_low')

print(IOstring)
content = IOstring.replace("\n", "/")

A = content.split('/')



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

