#include <bits/stdc++.h>
#include <sstream>
using namespace std;


int main()
{
    freopen("in.txt", "r", stdin);
    freopen("out.txt", "w", stdout);
    string line;
    bool id = false;
    while(getline(cin, line)){
        stringstream ss(line);
        int cnt = 0;
        int ans;
        string now;
        bool flag = false;
        while(ss>>now){
            cnt++;
            if(now == "ID]"&&id == false){
                id = true;
                break;
            }
            else if(now == "ID]"&&id){
                id = false;
                break;
            }
            if(cnt == 8){
                ss>>ans;
                //cout<<ans<<endl;
                flag = true;
            }
            if(flag){

                if(id&&now[0]!='-')
                    cout<<ans<<endl;
                flag = false;
            }

        }
    }
    return 0;
}
