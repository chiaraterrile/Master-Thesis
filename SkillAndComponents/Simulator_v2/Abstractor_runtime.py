from functions import TracesCreator
class Query:
    def __init__(self, **entries): self.__dict__.update(entries)


class Input_data :
  def __init__(self, name, index):
    self.name = name
    self.index = index


def RemoveElementByName (lista, string) :
    for i in range(0,len(lista)):
        if lista[i].name == string :
            index_found = i
            lista.pop(index_found)
    return lista

def FindIndexinList(lista,string) :
    for i in range (0,len(lista)):
        if lista[i].name == string :
            index_found = lista[i].index
        return index_found

def IsinList(lista,string) :
    for i in range (0,len(lista)):
        isIn = False
        if lista[i].name == string :
            isIn = True
        return isIn



#print(len(lista))
#lista = RemoveElementByName(lista,'q3')
#isIn = IsinList(lista,'q')
#print(isIn)
#index_data = FindIndexinList(lista,'q')
#print(index_data)


def Abstract2(log) :
    indexes = []
    flag = False

    index_temp = []
    global input_list
    input_list = []

    for i in range(0,len(log)):
        
        if log[i] == q1.input :
            isIn = IsinList(input_list,'q1')
            if isIn:
                index_temp = FindIndexinList(input_list,'q1')
                isIn = False
            else :
                index_temp = i
                input_list.append(Input_data('q1',index_temp))
            for j in range (i,len(log)) :
                if log[j] == q1.output and q1.output != q1.last :
                    indexes.append(index_temp)
                    indexes.append(j)
                    q1.last = q1.output
                    q3.last = "None"
                    input_list = RemoveElementByName(input_list,'q1')

                elif log[j] == q1.output and q1.output == q1.last :
                    input_list = RemoveElementByName(input_list,'q1')
                    
                
                    
                


        if log[i] == q3.input :
            isIn = IsinList(input_list,'q3')
            if isIn :
                index_temp = FindIndexinList(input_list,'q3')
                isIn = False
            else :
                index_temp = i
                input_list.append(Input_data('q3',index_temp))
            for j in range (i,len(log)) :
                if log[j] == q3.output and q3.output != q3.last :
                    indexes.append(index_temp)
                    indexes.append(j)
                    q3.last = q3.output
                    q1.last = "None"
                    q2.last = "None"
                    input_list = RemoveElementByName(input_list,'q3')

                elif log[j] == q3.output and q3.output == q3.last :
                    input_list = RemoveElementByName(input_list,'q3')
                    
               
                    
                

        if log[i] == q7.input :
            isIn = IsinList(input_list,'q7')
            if isIn :
                index_temp = FindIndexinList(input_list,'q7')
                isIn = False
            else :
                index_temp = i
                input_list.append(Input_data('q7',index_temp))
            for j in range (i,len(log)) :
                if log[j] == q7.output and q7.output != q7.last :
                    indexes.append(index_temp)
                    indexes.append(j)
                    q7.last = q7.output
                    q9.last = "None"
                    input_list = RemoveElementByName(input_list,'q7')

                elif log[j] == q7.output and q7.output == q7.last :
                    input_list = RemoveElementByName(input_list,'q7')
                    
                
                    

        if log[i] == q9.input :
            isIn = IsinList(input_list,'q9')
            if isIn :
                index_temp = FindIndexinList(input_list,'q9')
                isIn = False
            else :
                index_temp = i
                input_list.append(Input_data('q9',index_temp))
            for j in range (i,len(log)) :
                if log[j] == q9.output and q9.output != q9.last :
                    indexes.append(index_temp)
                    indexes.append(j)
                    q9.last = q9.output
                    q7.last = "None"
                    q8.last = "None"
                    input_list = RemoveElementByName(input_list,'q9')

                elif log[j] == q9.output and q9.output == q9.last :
                    input_list = RemoveElementByName(input_list,'q9')
                    
               
                    
                    
        
        
        if log[i] == q2.input :
            flag = False
            isIn = IsinList(input_list,'q2')
            if isIn :
                index_temp = FindIndexinList(input_list,'q2')
                isIn = False
            else :
                index_temp = i
                input_list.append(Input_data('q2',index_temp))
            for j in range (i,len(log)) :
                for h in range (0,len(q2.output)) :
                    if log[j] == q2.output[h] and q2.output[h] != q2.last :
                        indexes.append(index_temp)
                        indexes.append(j)
                        q2.last = q2.output[h]
                        flag = True
                        input_list = RemoveElementByName(input_list,'q2')
                    elif log[j] == q2.output[h] and q2.output[h] == q2.last:
                        input_list = RemoveElementByName(input_list,'q2')
                        flag = True
                    

                if flag == True :
                    break


        
        if log[i] == q4.input :
            flag = False
            isIn = IsinList(input_list,'q4')
            if isIn :
                index_temp = FindIndexinList(input_list,'q4')
                isIn = False
            else :
                index_temp = i
                input_list.append(Input_data('q4',index_temp))
            for j in range (i,len(log)) :
                for h in range (0,len(q4.output)) :
                    if log[j] == q4.output[h] and q4.output[h] != q4.last :
                        indexes.append(index_temp)
                        indexes.append(j)
                        q4.last = q4.output[h]
                        flag = True
                        input_list = RemoveElementByName(input_list,'q4')
                    elif log[j] == q4.output[h] and q4.output[h] == q4.last:
                        input_list = RemoveElementByName(input_list,'q4')
                        flag = True
                if flag == True :
                    break


        if log[i] == q5.input :
                flag = False
                isIn = IsinList(input_list,'q5')
                if isIn :
                    index_temp = FindIndexinList(input_list,'q5')
                    isIn = False
                else :
                    index_temp = i
                    input_list.append(Input_data('q5',index_temp))
                for j in range (i,len(log)) :
                    for h in range (0,len(q5.output)) :
                        if log[j] == q5.output[h] and q5.output[h] != q5.last :
                            indexes.append(index_temp)
                            indexes.append(j)
                            q5.last = q5.output[h]
                            flag = True
                            input_list = RemoveElementByName(input_list,'q5')
                        elif log[j] == q5.output[h] and q5.output[h] == q5.last:
                            input_list = RemoveElementByName(input_list,'q5')
                            flag = True

                    if flag == True :
                        break


        if log[i] == q6.input :
                flag = False
                isIn = IsinList(input_list,'q6')
                if isIn :
                    index_temp = FindIndexinList(input_list,'q6')
                    isIn = False
                else :
                    index_temp = i
                    input_list.append(Input_data('q6',index_temp))
                for j in range (i,len(log)) :
                    for h in range (0,len(q6.output)) :
                        if log[j] == q6.output[h] and q6.output[h] != q6.last :
                            indexes.append(index_temp)
                            indexes.append(j)
                            q6.last = q6.output[h]
                            flag = True
                            input_list = RemoveElementByName(input_list,'q6')
                        elif log[j] == q6.output[h] and q6.output[h] == q6.last:
                            input_list = RemoveElementByName(input_list,'q6')
                            flag = True

                    if flag == True :
                        break
        
        if log[i] == q8.input :
                flag = False
                isIn = IsinList(input_list,'q8')
                if isIn :
                    index_temp = FindIndexinList(input_list,'q8')
                    isIn = False
                else :
                    index_temp = i
                    input_list.append(Input_data('q8',index_temp))
                for j in range (i,len(log)) :
                    for h in range (0,len(q8.output)) :
                        if log[j] == q8.output[h] and q8.output[h] != q8.last :
                            indexes.append(index_temp)
                            indexes.append(j)
                            q8.last = q8.output[h]
                            flag = True
                            input_list = RemoveElementByName(input_list,'q8')
                        elif log[j] == q8.output[h] and q8.output[h] == q8.last:
                            input_list = RemoveElementByName(input_list,'q8')
                            flag = True

                    if flag == True :
                        break

        if log[i] == q10.input :
                flag = False
                isIn = IsinList(input_list,'q10')
                if isIn :
                    index_temp = FindIndexinList(input_list,'q10')
                    isIn = False
                else :
                    index_temp = i
                    input_list.append(Input_data('q10',index_temp))
                for j in range (i,len(log)) :
                    for h in range (0,len(q10.output)) :
                        if log[j] == q10.output[h] and q10.output[h] != q10.last :
                            indexes.append(index_temp)
                            indexes.append(j)
                            q10.last = q10.output[h]
                            flag = True
                            input_list = RemoveElementByName(input_list,'q10')
                        elif log[j] == q10.output[h] and q10.output[h] == q10.last:
                            input_list = RemoveElementByName(input_list,'q10')
                            flag = True

                    if flag == True :
                        break

        

    indexes.sort()
    new_log = []
    for i in range (0,len(indexes)) :
        new_log.append(log[indexes[i]])
    
    return new_log


def Abstract(log) :
    indexes = []
    flag = False

    index_temp = []
    
    for i in range(0,len(log)):
        if log[i] == q1.input :
                index_temp = i
                for j in range (i,len(log)) :
                    if log[j] == q1.output and q1.output != q1.last :
                        indexes.append(index_temp)
                        indexes.append(j)
                        q1.last = q1.output
                        q3.last = "None"

        if log[i] == q3.input :
            index_temp = i
            for j in range (i,len(log)) :
                if log[j] == q3.output and q3.output != q3.last :
                    indexes.append(index_temp)
                    indexes.append(j)
                    q3.last = q3.output
                    q1.last = "None"
                    q2.last = "None"
                

        if log[i] == q7.input :
            index_temp = i
            for j in range (i,len(log)) :
                if log[j] == q7.output and q7.output != q7.last :
                    indexes.append(index_temp)
                    indexes.append(j)
                    q7.last = q7.output
                    q9.last = "None"

        if log[i] == q9.input :
            index_temp = i
            for j in range (i,len(log)) :
                if log[j] == q9.output and q9.output != q9.last :
                    indexes.append(index_temp)
                    indexes.append(j)
                    q9.last = q9.output
                    q7.last = "None"
                    q8.last = "None"
                    
        
        
        if log[i] == q2.input :
            flag = False
            index_temp = i
            for j in range (i,len(log)) :
                for h in range (0,len(q2.output)) :
                    if log[j] == q2.output[h] and q2.output[h] != q2.last :
                        indexes.append(index_temp)
                        indexes.append(j)
                        q2.last = q2.output[h]
                        flag = True
                    elif log[j] == q2.output[h] and q2.output[h] == q2.last:
                        flag = True

                if flag == True :
                    break


        
        if log[i] == q4.input :
            flag = False
            index_temp = i
            for j in range (i,len(log)) :
                for h in range (0,len(q4.output)) :
                    if log[j] == q4.output[h] and q4.output[h] != q4.last :
                        indexes.append(index_temp)
                        indexes.append(j)
                        q4.last = q4.output[h]
                        flag = True
                    elif log[j] == q4.output[h] and q4.output[h] == q4.last:
                        flag = True
                if flag == True :
                    break


        if log[i] == q5.input :
                flag = False
                index_temp = i
                for j in range (i,len(log)) :
                    for h in range (0,len(q5.output)) :
                        if log[j] == q5.output[h] and q5.output[h] != q5.last :
                            indexes.append(index_temp)
                            indexes.append(j)
                            q5.last = q5.output[h]
                            flag = True
                        elif log[j] == q5.output[h] and q5.output[h] == q5.last:
                            flag = True

                    if flag == True :
                        break


        if log[i] == q6.input :
                flag = False
                index_temp = i
                for j in range (i,len(log)) :
                    for h in range (0,len(q6.output)) :
                        if log[j] == q6.output[h] and q6.output[h] != q6.last :
                            indexes.append(index_temp)
                            indexes.append(j)
                            q6.last = q6.output[h]
                            flag = True
                        elif log[j] == q6.output[h] and q6.output[h] == q6.last:
                            flag = True

                    if flag == True :
                        break
        
        if log[i] == q8.input :
                flag = False
                index_temp = i
                for j in range (i,len(log)) :
                    for h in range (0,len(q8.output)) :
                        if log[j] == q8.output[h] and q8.output[h] != q8.last :
                            indexes.append(index_temp)
                            indexes.append(j)
                            q8.last = q8.output[h]
                            flag = True
                        elif log[j] == q8.output[h] and q8.output[h] == q8.last:
                            flag = True

                    if flag == True :
                        break

        if log[i] == q10.input :
                flag = False
                index_temp = i
                for j in range (i,len(log)) :
                    for h in range (0,len(q10.output)) :
                        if log[j] == q10.output[h] and q10.output[h] != q10.last :
                            indexes.append(index_temp)
                            indexes.append(j)
                            q10.last = q10.output[h]
                            flag = True
                        elif log[j] == q10.output[h] and q10.output[h] == q10.last:
                            flag = True

                    if flag == True :
                        break

    indexes.sort()
    new_log = []
    for i in range (0,len(indexes)) :
        new_log.append(log[indexes[i]])
    
    return new_log


def IO_Split ():
    f = open('AbstractTraces_RT.txt', 'r')
    x = f.read()


    IOstring = x.replace('input','i/OK')
    IOstring = IOstring.replace('output','Delta/o')

    IOstring = IOstring.replace("\n", "/")

    A = IOstring.split('/')

    input_txt = A[0] + '\n'
    output_txt = A[1] + '\n'
    
    for i in range(1,int(len(A)/2)):
        
        input_txt = input_txt+A[2*i]+'\n'
    
    
    for i in range(1,int(len(A)/2)):
        output_txt = output_txt+ A[2*i+1] +'\n'

    with open("Input.txt", "w") as text_file:
        text_file.write(input_txt)

    with open("Ouput.txt", "w") as text_file:
        text_file.write(output_txt)


def Convert(string):
    li = list(string.split("\n"))
    return li

def FileOpenTraces ():
    f = open('Traces.txt', 'r')
    content = f.read()
    f.close()
    log_list = Convert(content)
    return log_list

q1 = Query(input = "goTo_ki_input",output =("output_goto_ki"),last = "None")

q2 = Query(input = "getStatus_ki_input",output=("output_getStatus_ki_ab","output_getStatus_ki_run","output_getStatus_ki_suc"),last = "None")

q3 = Query(input = "halt_ki_input",output="output_halt_ki",last = "None")

q4 = Query(input = "isAt_ki_input",output=("output_isAt_ki_false","output_isAt_ki_true"),last = "None")

q5 = Query(input = "batteryStatus_input",output=("output_batteryStatus_false","output_batteryStatus_true"),last = "None")

q6 = Query(input = "level_input",output=("output_level_low","output_level_medium_one","output_level_medium_two","output_level_medium_three","output_level_medium_four","output_level_medium_five","output_level_medium_six","output_level_high","output_level_zero"),last = "None")

q7 = Query(input = "goTo_ch_input",output="output_goto_ch",last = "None")

q8 = Query(input = "getStatus_ch_input",output=("output_getStatus_ch_ab","output_getStatus_ch_run","output_getStatus_ch_suc"),last = "None")

q9 = Query(input = "halt_ch_input",output="output_halt_ch",last = "None")

q10 = Query(input = "isAt_ch_input",output=("output_isAt_ch_false","output_isAt_ch_true"),last = "None")




index_temp = []
new_log= []


   

N = 200
# take only N messages per time
#for i in range(0,(int(len(log_list)/N))) :
i = 0
while(1):
    TracesCreator()
    log_list = FileOpenTraces()
    temp_log = log_list[N*i:N*i + N -1]
    new_log = new_log + Abstract(temp_log)
    with open("AbstractTraces_RT.txt", "w") as text_file:
        text_file.write('\n'.join(map(str, new_log)))
    IO_Split()
    if len(log_list) < (N*(i+1) + N) :
        while(len(log_list)< (N*(i+1) + N)) :
            log_list = FileOpenTraces()
            print('Waiting for new messages')
    i = i+1   
    #print(i)

#temp_log = log_list[len(log_list)-int(len(log_list)/N)+1:len(log_list)]
#new_log = new_log + Abstract(temp_log)
#with open("AbstractTraces_RT.txt", "w") as text_file:
 #   text_file.write('\n'.join(map(str, new_log)))
#IO_Split()