class Query:
    def __init__(self, **entries): self.__dict__.update(entries)

def Convert(string):
    li = list(string.split("\n"))
    return li


q1 = Query(input = "goTo_ki_input",output =("output_goto_ki"),last = "None")

q2 = Query(input = "getStatus_ki_input",output=("output_getStatus_ki_ab","output_getStatus_ki_run","output_getStatus_ki_suc"),last = "None")

q3 = Query(input = "halt_ki_input",output="output_halt_ki",last = "None")

q4 = Query(input = "isAt_ki_input",output=("output_isAt_ki_not","output_isAt_ki_run"),last = "None")

q5 = Query(input = "batteryStatus_input",output=("output_batteryStatus_false","output_batteryStatus_true"),last = "None")

q6 = Query(input = "level_input",output=("output_level_normal","output_level_low"),last = "None")

q7 = Query(input = "goTo_ch_input",output="output_goto_ch",last = "None")

q8 = Query(input = "getStatus_ch_input",output=("output_getStatus_ch_ab","output_getStatus_ch_run","output_getStatus_ch_suc"),last = "None")

q9 = Query(input = "halt_ch_input",output="output_halt_ch",last = "None")

q10 = Query(input = "isAt_ch_input",output=("output_isAt_ch_not","output_isAt_ch_run"),last = "None")


f = open('Traces.txt', 'r')
content = f.read()
f.close()

log = Convert(content)

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

    if log[i] == q9.input :
        index_temp = i
        for j in range (i,len(log)) :
            if log[j] == q9.output and q9.output != q9.last :
                indexes.append(index_temp)
                indexes.append(j)
                q9.last = q3.output
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
#print(new_log)

with open("AbstractTraces.txt", "w") as text_file:
    text_file.write('\n'.join(map(str, new_log)))


