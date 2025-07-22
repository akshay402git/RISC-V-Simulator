#include <iostream>
#include <fstream>
#include <map>

using namespace std;

void store(map<string,int> & reg,char c,int a,int b,int start = 0) // function to pair all register names with their code
{
    string x; x += c;
    for(int i = a; i <= b; i++)
    {
        x += to_string(i-a+start);
        reg[x] = i;
        while(x.size() > 1)  x.pop_back();
    }
}

bool check(int& immediate,string s,string func) /* checks if s is a proper constant and
                                                   updates it to immediate according to the format */

{
    bool res = 1,hexadecimal = 0,first = 1,first1 = 1; int val;
    if(s.size() > 12) return 0;
    for(int i = 0; i < s.size(); i++) // parsing the string and verifying it is a number
    {
        if(s[i] == 'x')
        {
            if(i && s[i-1] == '0')
            {
                if(i-1)
                {
                    if(s[i-2] != '-') return 0;
                }
                hexadecimal = true;
            }
            else return 0;
        }
        else if(s[i] == '-' && !i) ;
        
        else if(hexadecimal && (((s[i] <= 'f') && (s[i] >= 'a')) || (s[i] <= '9') && (s[i] >= '0'))) ;
        
        else if((s[i] <= '9') && (s[i] >= '0')) ;
        
        else return 0;
    }
    
   
    if(func == "lui") // if instruction is lui then 32 bit values allowed
    {
        long v = (hexadecimal ? stol(s,nullptr,16) : stol(s));
        if(v < 0 || v >= (1LL << 32)) return 0;
        val = v % (1 << 20);
    }
    
    else val = (hexadecimal ? stoi(s,nullptr,16) : stoi(s)); // converting string to integer
    
    if(func[0] != 'b' && (func[0] != 'j' || func.back() == 'r')) // checks for valid range in I type(addi etc.)
    {
        if(func != "lui" && (val < -(1 << 11) || val >= (1 << 11))) return 0;
    }
    
    if(func[0] == 'b' && (val < -(1 << 12) || val >= (1 << 12))) return 0; // checks for valid range in branch
    if(func[0] == 'j' && (val < -(1 << 20) || val >= (1 << 20))) return 0; // chekcs for valid range in jump
    
    if(func[0] == 's' && func.back() == 'i')  if(val < 0 || val > 63) return 0; // checks for valid range in shift
    
    if(func[0] == 'b') immediate = (( val < 0 ? val + (1 << 13) : val) >> 1); // 2's complement for appropriate instr.
    
    else if(func[0] == 'j' && func.back() != 'r') immediate = ((val < 0 ? val + (1 << 21) : val) >> 1);
       
    else if(func == "lui") immediate = val;
    
    else immediate = ( val < 0 ? val + (1 << 12) : val);
    
    if(func[0] == 's' && func.back() == 'i')
    {
        immediate %= (1 << 6);
        if(func == "srai")
        {
            immediate += 16*(1 << 6);
        }
    }
    
    return 1;
}
string hex(unsigned int val)  // converts 32 bit unsigned into 8 digit hex code
{
    string result;
    for(int _ = 0; _ < 8; _ ++)
    {
        int r = val % 16;
        if(r < 10)
        {
            result += to_string(r);
        }
        else
        {
            result += ('a' + r%10);
        }
        val /= 16;
    }
    reverse(result.begin(),result.end());
    return result;
}

/*GLOBAL VARIABLES */

map<string,int> reg,label,I[6]; unsigned int mcode; // reg maps register names with reg no. , label with line no.
string o[4]; int immediate,line_ct = 0,p_ct = 0;    // mcode stores 32 bit machine code , o stores the words in each line
                                                    // line_ct keeps count of line number in input file, p_ct of program.
/* ERROR HANDLING */
void rs(bool check)
{
    if(!check)
    {
        cout << "invalid source register in line: " << line_ct << endl;
        exit(1);
    }
}
void rd(bool check)
{
    if(!check)
    {
        cout << "invalid destination register in line: " << line_ct << endl;
        exit(1);
    }
}
void lb(bool check)
{
    if(!check)
    {
        cout << "invalid label or offset or immediate value in line: " << line_ct << endl;
        exit(1);
    }
}

void op(bool check)
{
    if(!check)
    {
        cout << "invalid number of operands in line: " << line_ct << endl;
        exit(1);
    }
}
void op1(bool check)
{
    if(!check)
    {
        cout << "invalid operation or operand in line: " << line_ct << endl;
        exit(1);
    }
}
void const label_check(string &s)  // rules for labels same as variables
{
    lb(!label.count(s)); // checks for duplicate label
     for(int i = 0; i < s.size(); i++) // only underscore and alphanumeric characters allowed
     {
         if(s[i] == '_');
         else if(s[i] >= 'a' && s[i] <= 'z');
         else if(s[i] >= 'A' && s[i] <= 'Z');
         else if(s[i] >= '0' && s[i] <= '9' && i); // digits can't be at the start
         else lb(0);
     }
}

/* HEX CODE GENERATION FOR VARIOUS FORMATS */
void R()
{
    rd(reg.count(o[1]));  rs(reg.count(o[2]) && reg.count(o[3])); // checking if appropriate words are valid
                    
    mcode += (I[0][o[0]] >> 3);  mcode <<= 5; // funct7
    mcode += reg[o[3]];  mcode <<= 5; // rs2
    mcode += reg[o[2]];  mcode <<= 3; // rs1
    mcode += I[0][o[0]] % (1 << 3); mcode <<= 5; // funct3
    mcode += reg[o[1]]; mcode <<= 7; // rd
    mcode += 51; // opcode
}
void IS()
{
    if(o[0][0] == 'l' || (o[0][0] == 's' && o[0].back() != 'i')) // load and store operations
    {
        op(o[3].empty());
        (o[0][0] == 'l' ? rd(reg.count(o[1])) : rs(reg.count(o[1])));
    
        for(int i = 0; i < o[2].size(); i++)
        {
            if(o[2][i] == '('){o[2] = o[2].substr(i,o[2].size() - i);break;}
            o[3] += o[2][i];
        }

        op1(!o[3].empty() && o[2].size() >= 4 && o[2][0] == '(' && o[2].back() == ')');
    
        o[2].erase(o[2].begin());  o[2].pop_back();
        rs(reg.count(o[1]));
   
        lb(check(immediate,o[3],o[0])); // checking if value is valid and storing it in immediate
    }

    else // arithmetic with constants
    {
        rd(reg.count(o[1])); rs(reg.count(o[2]));
        if(o[0][0] == 'j' && label.count(o[3]))
        {
            immediate = (label[o[3]])*4;
            lb(check(immediate,to_string(immediate),o[0]));
        }
        else  lb(check(immediate,o[3],o[0]));
        
    }

    if(o[0].back() == 'i' || o[0][0] == 'l' || o[0][0] == 'j')
    {
        mcode += immediate; mcode <<= 5; // immediate[11:0]
        mcode += reg[o[2]]; mcode <<= 3; // rs1
        mcode += I[1][o[0]]; mcode <<= 5; // funct3
        mcode += reg[o[1]]; mcode <<= 7; // rd
        mcode += (o[0].back() == 'i' ? 19 : 3); // opcode
        mcode += (o[0][0] == 'j' ? 100 : 0);
    }
           
    else
    {
        mcode += immediate / (1 << 5); mcode <<= 5; // immediate[11:5]
        mcode += reg[o[1]]; mcode <<= 5; // rs2
        mcode += reg[o[2]]; mcode <<= 3; // rs1
        mcode += I[2][o[0]]; mcode <<= 5; // funct3
        mcode += immediate % (1 << 5); mcode <<= 7; // immediate[4:0]
        mcode += 35; // opcode
    }
}
        
void B()
{
    rs(reg.count(o[1]) && reg.count(o[2]));
    lb(check(immediate,o[3],o[0]) || label.count(o[3]));
                          
    if(label.count(o[3])) // case where label is provided
    {
        int val = (label[o[3]] - p_ct)*4; // calculating offset
        lb(val >= -(1 << 12) && val < (1 << 12));
        immediate = ((val < 0 ? val + (1 << 13) : val) >> 1);
    }
                              
    int last2 = (immediate >> 10);  //  storing MS 2 bits
    mcode += last2 / 2; mcode <<= 6; // immediate[12]
    immediate %= (1 << 10);
            
    mcode += immediate >> 4; mcode <<= 5; // immediate[12|10:5]
    mcode += reg[o[2]]; mcode <<= 5; // rs2
    mcode += reg[o[1]]; mcode <<= 3; // rs1
    mcode += I[3][o[0]]; mcode <<= 5; // funct3
    
    mcode += ((immediate % (1 << 4)) << 1) + (last2 % 2); // immediate[4:1|11]
    mcode <<= 7; mcode += 99; // opcode
}
void J()
{
    op(o[3].empty()); rs(reg.count(o[1]));
    lb(check(immediate,o[2],o[0]) || label.count(o[2]));
       
    if(label.count(o[2]))
    {
        int val = (label[o[2]] - p_ct)*4;
        lb(val >= -(1 << 20) && val < (1 << 20));
        immediate = (( val < 0 ? val + (1 << 21) : val) >> 1);
    }
    
    mcode += immediate >> 19; mcode <<= 10; immediate %= (1 << 19); // immediate[20]
    mcode += immediate % (1 << 10); // immediate[20|10:1]
    immediate >>= 10; mcode <<= 1;
    mcode += immediate % 2; mcode <<= 8; // immediate[20|10:1|11]
    mcode += immediate >> 1; mcode <<= 5; // immediate[20|10:1|11|19:12]
    mcode += reg[o[1]]; mcode <<= 7; // rd
    mcode += 111; // opcode
}
void U()
{
    op(o[3].empty()); rs(reg.count(o[1]));
    lb(check(immediate,o[2],o[0])); 
    
    mcode += immediate; mcode <<= 5; // immediate[31:12]
    mcode += reg[o[1]]; mcode <<= 7; // rd
    mcode += 55; // opcode
    
}

void initialise()
{
    store(reg,'x',0,31); // initialising register names
    store(reg,'t',5,7);
    store(reg,'a',10,17);
    store(reg,'s',18,27,2);
    store(reg,'t',28,31,3);
    
    reg["zero"] = 0 , reg["ra"] = 1 , reg["sp"] = 2 , reg["gp"] = 3; // custom register names
    reg["tp"] = 4 , reg["s0"] = 8 , reg["fp"] = 8 , reg["s1"] = 9;
    
    
    I[0]["add"] = 0 , I[0]["and"] = 7 ,  I[0]["sub"] = (32 << 3);  // R - format instructions
    I[0]["xor"] = 4 , I[0]["or"] = 6 , I[0]["sll"] = 1;
    I[0]["srl"] = 5; I[0]["sra"] = (32 << 3) + 5;
    
    I[1]["addi"] = 0 , I[1]["xori"] = 4 , I[1]["andi"] = 7 , I[1]["ori"] = 6; I[1]["jalr"] = 0;// I - format instructions
    I[1]["slli"] = (0 << 3) + 1 , I[1]["srli"] = (0 << 3) + 5 , I[1]["srai"] = (0 << 3) + 5;
    
    I[1]["lb"] = 0 , I[1]["lh"] = 1 , I[1]["lw"] = 2 , I[1]["ld"] = 3;
    I[1]["lbu"] = 4 , I[1]["lhu"] = 5 , I[1]["lwu"] = 6;
    
    I[2]["sb"] = 0 , I[2]["sh"] = 1 , I[2]["sw"] = 2 , I[2]["sd"] = 3; // S - format instructions
    
    I[3]["beq"] = 0 , I[3]["bne"] = 1 , I[3]["blt"] = 4 ;  // B - format instructions
    I[3]["bge"] = 5 , I[3]["bltu"] = 6 , I[3]["bgeu"] = 7 ;
    
    I[4]["jal"] = 0; // J - format  instruction
    I[5]["lui"] = 0; // U - format  instruction
    
}
void option(int c) // option to choose respective function for each type of instruction
{
    mcode = 0;
    switch(c)
    {
        case 0:
                R();  break;
        case 1:
        case 2:
                IS(); break;
        case 3:
                B();  break;
              
        case 4:
                J();  break;
        case 5:
                U();
    }
}

int main()
{
    vector<string> line; string temp;
    initialise(); // creates all register names and maps instructions with values
    
    ifstream infile("input.s");
    ofstream out("output.hex");
   
    while(!infile.eof()) // taking input and storing it
    {
        getline(infile,temp);
        if(!infile.eof() && temp != "")
        line.push_back(temp);
    }

    for(line_ct = 1,p_ct = 0; line_ct < line.size()+1; line_ct++) // storing labels
    {
        temp.clear();
        for(auto & c: line[line_ct-1])
        {
            if(c == ':')
            {
                label_check(temp);
                label[temp] = p_ct;
                temp.clear();
                continue;
            }
            
            if(c != ' ')
            temp += c;
        }
        p_ct += (line[line_ct-1].back() != ':');
    }
    
    auto Temp = label;
    for(auto &i: Temp) if(i.second == p_ct) label.erase(i.first);
    
    for(line_ct = 1,p_ct = 0; line_ct < line.size()+1; line_ct++ ) // parsing the file line by line
    {
        for(int _ = 0; _ < 4; _ ++) o[_].clear();
        int choice = 0;
        
        for(auto & c: line[line_ct-1]) // Splitting the line into words
        {
            if(c == ' ')
            {
                if(choice || o[0].back() != ':')choice++;
                else o[0].clear();
                continue;
            }
            
            if(choice < 4) o[choice] += c;
            else op(0);
        }
        
        if(!choice && line[line_ct-1].back() == ':')  continue;
    
        for(int z = 0; z <= choice; z++)
        {
            if(o[z].back() == ',') o[z].pop_back();
            if(o[z][0] == '(')
            {
                if(o[z].back() != ')') op1(0);
                else
                {
                    o[z].erase(o[z].begin());
                    o[z].pop_back();
                }
                
            }
        }
       
        for(int j = 0; j < 6; j++)
        {
            if(I[j].count(o[0])) // decoding type of instruction and printing hex code in output.hex
            {
                option(j); p_ct++;
                out << hex(mcode) << endl;
                break;
            }
            op1(j != 5);
        }
       
    }
    
    return 0;
}

                   

                       

                


