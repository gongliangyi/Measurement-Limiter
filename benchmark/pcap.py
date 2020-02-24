import json
import sys

def write_txt(lst):
    with open('line.txt', 'w') as f:
        length = len(lst)
        for i in range(length):
            f.write(str(lst[i])+'\n')



if __name__ == "__main__":
    
    fd = open('latency.txt', mode = 'w',encoding='utf-8')
    record = {}
    send_number = 0
    send_number2 = 0
    send_number3 = 0
    f = open("./receive.json", "r", encoding = "utf-8")
    data = json.load(f)
    ans = []
    send_number1 = 0
    seq_num = []
    time_stamp = []
    time_stamp_mp = {}
    time_stamp_idx = {}
    same = 0
    
    for item in data:
        
#        if 'data' not in item['_source']['layers'].keys():
#            continue
        if item['_source']['layers']['frame']['frame.time_epoch'] is None:
            continue
        if 'tcp' not in item['_source']['layers'].keys():
            continue
        val = item['_source']['layers']['tcp']['tcp.options']
        if 'ip' not in item['_source']['layers'].keys():
            continue
        if item['_source']['layers']['ip']['ip.src'] == "45.32.222.78" and item['_source']['layers']['ip']['ip.dst'] == "66.42.83.241":
            send_number1 += 1
        timestamp = eval(item['_source']['layers']['frame']['frame.time_epoch'])
        
        hash_val = hash(val)
        if hash_val in time_stamp_mp.keys():
            same+=1
        else:
            time_stamp_mp[hash_val] = timestamp
            
        
    f = open("./send.json", "r", encoding = "utf-8")
    data = json.load(f)
    vis = {}
    same1=0
    front = 0
    for item in data:
        if item['_source']['layers']['frame']['frame.time_epoch'] is None:
            continue
        
#        if 'data' not in item['_source']['layers'].keys():
#            continue
#        print(item['_source']['layers']['ip']['ip.src'])
        if 'tcp' not in item['_source']['layers'].keys():
            continue
        val = item['_source']['layers']['tcp']['tcp.options']
        if 'ip' not in item['_source']['layers'].keys():
            continue
        if item['_source']['layers']['ip']['ip.src'] == "45.32.222.78" and item['_source']['layers']['ip']['ip.dst'] == "66.42.83.241":
            send_number += 1
        #hash_val = hash(item['_source']['layers']['data']['data.data'])
        timestamp = eval(item['_source']['layers']['frame']['frame.time_epoch'])
        hash_val = hash(val)
        #give up some small packets because they all are same and hard to figure out
        if front < 20:
            front += 1
            continue
        if val not in vis.keys():
            print("%.4f"%(abs(timestamp-time_stamp_mp[hash_val])*2*1000), file=fd)
            vis[hash_val] = True
        else:
            same1+=1
    print(same)
    print(same1)
    fd.close()
    