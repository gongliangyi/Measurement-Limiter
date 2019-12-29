#include<cstdio>
#include<cstring>
#include<algorithm>
#include<iostream>
#include<queue>
#include<cmath>
#include<map>
#include<stack>
#include<set>
#include<bitset>
#include <sstream>

using namespace std;

vector<int> byte;
int main(){
    ios::sync_with_stdio(false);

    freopen("iperf_udp_5m.txt", "r", stdin);
    freopen("out3.txt", "w", stdout);
    int b;
    string s;
    bool flag = false;
    int tot = 0;
    int now;
    while(getline(cin, s)){
        stringstream ss(s);
        ss>>now;
        if(!flag){
            flag = true;
            tot+=now;
        }
        else{
            tot+=now;
        }
        if(now == 0&&tot!=0){
            byte.push_back(tot);
            tot = 0;
            flag = false;
        }
    }
    for(int i=0; i<byte.size(); i++){
        cout<<byte[i]<<endl;
    }



    return 0;
}


