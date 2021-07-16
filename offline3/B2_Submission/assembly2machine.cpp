#include <bits/stdc++.h>
#define ll long long

using namespace std;

ll str2dec(string str)
{
    /**
        converts string to decimal integer
        if the value is negative, outputs 256 + value
    */

    ll dec = 0;

    for(int i = 0; str[i]; i++){
        if(str[i] == '-')continue;
        dec *= 10;
        dec += (str[i] - '0');
    }

    if(str[0] == '-'){
        // negative, applying 2's complement
        dec = 256 - dec;    // considering 8 bit
    }

    return dec;
}

string to_hex(ll dec)
{
    /**
        converts 0-15 to hex
        throws input out of bound exception
    */

    if(dec < 0 or dec > 15) throw "Error: input out of bound, to_hex() can only work with 0-15 as input";

    string hex = "";

    hex += (((dec%16 > 9)?'A'-10:'0') + (dec%16));

    return hex;
}

string dec2hex(ll dec)
{
    /**
        converts unsigned decimal numbers to hex
    */

    string hex = "";

    while(dec)
    {
        hex += (((dec%16 > 9)?'A'-10:'0') + (dec%16));
        dec /= 16;
    }

    reverse(hex.begin(), hex.end());

    if(!hex.length())hex = "0";

    return hex;
}

string removeComment(string line)
{
    /**
        removes single line comment initiating with "//" character sequence
    */

    int l = line.length();
    for(int i = 1; i < l; i++){
        if(line[i-1] == '/' and line[i] == '/'){
            return line.substr(0, i-1);
        }
    }

    return line;
}

string strip(string word)
{
    /**
        removes all occurances ' ', '\t' and/or '\n' characters from the input string
    */

    for(int i = 0; i < word.length(); i++){
        if(word[i] == ' ' or word[i] == '\t' or word[i] == '\n'){
            word.erase(word.begin()+i, word.begin() + i+1);
            i--;
        }
    }
    return word;
}

vector <string> split(string line, string delimiter = ",")
{
    /**
        splits input string (line) at the positions of delimiter and removes it
        generates a token list in order
    */

    vector <string> token_list;

    size_t pos = 0;

    while((pos = line.find(delimiter)) != string::npos) {
        token_list.push_back(strip(line.substr(0, pos)));
        line.erase(0, pos + delimiter.length());
    }

    token_list.push_back(strip(line));

    return token_list;
}

int main()
{
    fstream fin, ftemp, fout;
    fin.open("input.txt", ios::in);         // input assembly code file
    ftemp.open("temp.txt", ios::out);       // temporary file for refined code, deleted after use
    fout.open("machine_code.bin", ios::out);        // output bin file with header, to be directly loaded from logisim rom

    string word, line, hex;
    ll dec, PC = 0;

    map <string, ll> labels;
    map <string, ll> opcodes;
    set <string> r_format, i_format, j_format, stk;
    vector <string> tokens;
    map <string, ll> reg;

    reg = {
            {"$zero", 0},
            {"$t0", 1},
            {"$t1", 2},
            {"$t2", 3},
            {"$t3", 4},
            {"$t4", 5},
            {"$sp", 6}
        };

    opcodes =  {
                {"add", 3},
                {"addi", 7},
                {"sub", 11},
                {"subi", 4},
                {"and", 12},
                {"andi", 6},
                {"or", 5},
                {"ori", 15},
                {"sll", 2},
                {"srl", 9},
                {"nor", 8},
                {"sw", 1},
                {"lw", 13},
                {"beq", 10},
                {"bneq", 14},
                {"j", 0}
            };

    r_format = {"add", "sub", "and", "or", "sll", "srl", "nor"};
    i_format = {"addi", "subi", "andi", "ori", "sw", "lw", "beq", "bneq"};
    j_format = {"j"};
    stk = {"push", "pop"};


    // removing comments and marking labels and empty lines

    // initializing stack pointer
    //ftemp << "addi $sp, $zero, 0" << endl;

    PC = 0;
    while(!fin.eof()){
        getline(fin, line);
        line = removeComment(line);

        int i;
        for(i = 0; i < line.length(); i++){
            if(line[i] == ' ' or line[i] == '\t' or line[i] == '\n')break;
        }
        string opcode = line.substr(0, i);

        if(opcode == "push"){
            // stack stuff push

            string dest_reg = strip(line.substr(i, line.length()));

            if(dest_reg[dest_reg.length() - 1] == ')'){
                // push 3($t0) ; dest_reg = 3($t0)
                ftemp << "subi $sp, $sp, 2" << endl;
                PC++;
                ftemp << "sw $t0, 0($sp)" << endl;
                PC++;
                ftemp << "lw $t0, " << dest_reg << endl;
                PC++;
                ftemp << "sw $t0, 1($sp)" << endl;
                PC++;
                ftemp << "lw $t0, 0($sp)" << endl;
                PC++;
                line = "addi $sp, $sp, 1";
            }
            else{
                ftemp << "subi $sp, $sp, 1" << endl;
                PC++;
                line = "sw " + dest_reg + ", 0($sp)";
            }
        }
        else if(opcode == "pop"){
            // stack stuff pop
            ftemp << "lw " + strip(line.substr(i, line.length())) + ", 0($sp)" << endl;
            PC++;
            line = "addi $sp, $sp, 1";
        }
        else if(opcodes.find(opcode) == opcodes.end()){
            // label
            // store label address
            if(opcode[opcode.length()-1] == ':')opcode = opcode.substr(0, opcode.length()-1);

            labels[strip(opcode)] = PC;
            continue;
        }

        if(strip(line) == "")continue;
        ftemp << line << endl;
        PC++;
    }

    // printing identified labels to console
    for(auto label: labels){
        cout << label.first << ' ' << label.second << endl;
    }
    cout << endl << endl;

    // reopening temp.txt to move file pointer to the start of file and also as input stream
    ftemp.close();
    ftemp.open("temp.txt", ios::in);

    // header for logisim rom
    fout <<  "v2.0 raw" << endl;


    // reading from the modified file temp.text

    PC = 0;     // setting program counter to 0
    while(ftemp >> word)
    {
        getline(ftemp, line);
        tokens = split(line);

        //opcode
        hex = "" + to_hex(opcodes[word]);

        if(r_format.count(word)){
            //src 1
            hex += to_hex(reg[tokens[1]]);

            //src 2
            hex += to_hex(reg[tokens[2]]);

            //dest
            hex += to_hex(reg[tokens[0]]);

            //shift
            if(word == "sll" or word == "srl" ){
                hex += to_hex(str2dec(tokens[2]));
            }else hex += '0';
        }
        else if(i_format.count(word)){
            if(word == "lw" or word == "sw"){
                // process pseudo direct addressing

                string temp_str = tokens[1];
                tokens.pop_back();

                tokens.push_back(temp_str.substr(temp_str.length() - 4, 3));    //rs register
                tokens.push_back(strip(temp_str.substr(0, temp_str.length() - 5)));    //offset

            }
            //src
            hex += to_hex(reg[tokens[(word[0] == 'b')?0:1]]);

            //dest
            hex += to_hex(reg[tokens[(word[0] == 'b')?1:0]]);

            //Immediate
            ll temp = ((word[0] == 'b')? (labels[tokens[2]] - PC - 1): str2dec(tokens[2]));

            if(temp < 0)temp = 256 + temp;      // if negative, applying 2's complement

            hex += to_hex(temp/16) + to_hex(temp%16);
        }
        else if(j_format.count(word)){

            //Target Jump Address
            hex += to_hex(labels[tokens[0]]/16) + to_hex(labels[tokens[0]]%16);
            hex += "00";
        }


        cout << word << ' ' << line << '\t' << hex << endl;     // console check
        fout << hex << ' ';                                     // to the logisim file

        PC++;
    }


    fin.close();
    ftemp.close();
    fout.close();

    remove("temp.txt");

    return 0;
}
