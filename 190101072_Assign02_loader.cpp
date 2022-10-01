#include <bits/stdc++.h>

using namespace std;

string inputFile, outputFile;
long long progaddr, csaddr, execaddr, cslth, endaddr;
string line;
map<string, long long> ESTAB;
vector<pair<long long, long long>> textRecords;
bool error = false;

// Map to get length of instructions from opcodes
map<string, string> LENS;

void initialize_LENS()
{
    LENS["18"] = "3";
    LENS["58"] = "3";
    LENS["90"] = "2";
    LENS["40"] = "3";
    LENS["B4"] = "2";
    LENS["28"] = "3";
    LENS["88"] = "3";
    LENS["A0"] = "2";
    LENS["24"] = "3";
    LENS["64"] = "3";
    LENS["9C"] = "2";
    LENS["C4"] = "1";
    LENS["C0"] = "1";
    LENS["F4"] = "1";
    LENS["3C"] = "3";
    LENS["30"] = "3";
    LENS["34"] = "3";
    LENS["38"] = "3";
    LENS["48"] = "3";
    LENS["00"] = "3";
    LENS["68"] = "3";
    LENS["50"] = "3";
    LENS["70"] = "3";
    LENS["08"] = "3";
    LENS["6C"] = "3";
    LENS["74"] = "3";
    LENS["04"] = "3";
    LENS["D0"] = "3";
    LENS["20"] = "3";
    LENS["60"] = "3";
    LENS["98"] = "2";
    LENS["C8"] = "1";
    LENS["44"] = "3";
    LENS["D8"] = "3";
    LENS["AC"] = "2";
    LENS["4C"] = "3";
    LENS["A4"] = "2";
    LENS["A8"] = "2";
    LENS["F0"] = "1";
    LENS["EC"] = "3";
    LENS["0C"] = "3";
    LENS["78"] = "3";
    LENS["54"] = "3";
    LENS["80"] = "3";
    LENS["D4"] = "3";
    LENS["14"] = "3";
    LENS["7C"] = "3";
    LENS["E8"] = "3";
    LENS["84"] = "3";
    LENS["10"] = "3";
    LENS["1C"] = "3";
    LENS["5C"] = "3";
    LENS["94"] = "2";
    LENS["B0"] = "2";
    LENS["E0"] = "3";
    LENS["F8"] = "1";
    LENS["2C"] = "3";
    LENS["B8"] = "2";
    LENS["DC"] = "3";
}

// trim from left
inline std::string &ltrim(std::string &s, const char *t = " \t\n\r\f\v")
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from right
inline std::string &rtrim(std::string &s, const char *t = " \t\n\r\f\v")
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

string trimWhitespaces(string s)
{
    // Trims whitespaces from string
    const char *t = " \t\n\r\f\v";
    string tmp = ltrim(rtrim(s, t), t);
    return tmp;
}

long long hexToInteger(string s)
{
    // Helper function to convert HEX to INT
    long long num = 0;
    num = stoll(s, 0, 16);
    return num;
}

string integerToHex(long n)
{
    // Helper function to convert INT to HEX
    stringstream s;
    s << hex << n;
    string tmp = s.str();
    string res = "";
    for (auto ch : tmp)
    {
        res += toupper(ch);
    }
    return res;
}

string getHex2Comp(long long num, long long length)
{ // Get 2s complement form of num
    if (num >= 0)
    {
        return integerToHex(num);
    }
    else
    {
        return integerToHex((1 << length) + num);
    }
}

string padZeroesToLeft(string s, long long n)
{
    // Pads string with zeroes in the beginning
    if ((n - s.size()) <= 0)
    {
        return s;
    }
    string tmp(n - s.size(), '0');
    return tmp + s;
}

struct HeaderRecord
{ // Struct to parse Header records
    string secName;
    long long startingAddress;
    long long length;
    HeaderRecord(string &line)
    {
        secName = trimWhitespaces(line.substr(1, 6));
        startingAddress = hexToInteger(trimWhitespaces(line.substr(7, 6)));
        length = hexToInteger(trimWhitespaces(line.substr(13, 6)));
    }
};

class TextRecord
{
public:
    // Struct to parse text records and load to mmap
    long long startingAddress;
    long long length;
    string instructions;
    TextRecord(string &line)
    {
        startingAddress = hexToInteger(trimWhitespaces(line.substr(1, 6)));
        length = hexToInteger(trimWhitespaces(line.substr(7, 2)));
        instructions = line.substr(9);
        textRecords.push_back({(startingAddress + csaddr) * 2, length * 2});
    }

    void loadToMem(char mmap[])
    { // Loads the object code at the address in mmap
        long long sz = instructions.size();
        strncpy(mmap + (csaddr + startingAddress) * 2, instructions.c_str(), sz);
    }
};

class DefineRecord
{
public:
    // class to parse Define records
    vector<pair<string, long long>> defSyms;
    DefineRecord(string &line)
    {
        string symbol;
        long long address;
        for (long long i = 1; i < (int)(line.size()); i += 12)
        {
            symbol = trimWhitespaces(line.substr(i, 6));
            address = hexToInteger(trimWhitespaces(line.substr(i + 6, 6)));
            defSyms.push_back({symbol, address});
        }
    }
};

class ModificationRecord
{
public:
    // Struct to parse M records and make updates in mmap
    long long address, length;
    char flag;
    string extSym;
    ModificationRecord(string &line)
    {
        address = hexToInteger(trimWhitespaces(line.substr(1, 6)));
        length = hexToInteger(trimWhitespaces(line.substr(7, 2)));
        flag = line[9];
        extSym = trimWhitespaces(line.substr(10, 6));
    }

    void updateVal(char mmap[])
    { // Updates value at address in mmap
        if (flag == '+')
        {
            addDelta(ESTAB[extSym], mmap);
        }
        else
        {
            addDelta(-ESTAB[extSym], mmap);
        }
    }

    void addDelta(long long delta, char mmap[])
    {
        // Adds delta at the address in mmap
        char buf[length + 1];
        long long effAddress = (csaddr + address) * 2 + length % 2;
        strncpy(buf, mmap + effAddress, length); // Get bytes at address
        buf[length] = '\0';
        string s(buf);
        long long val = hexToInteger(s);
        val += delta; // Add delta to it
        string temp = getHex2Comp(val, length * 4);
        temp = padZeroesToLeft(temp, length);

        strncpy(mmap + effAddress, temp.c_str(), length); // Write the bytes back in mmap
    }
};

class EndRecord
{
    // class for parsing EndRecords
public:
    long long startingAddress;
    // constructor
    EndRecord(string &line)
    {
        if (line.size() == 1)
            return;
        startingAddress = hexToInteger(trimWhitespaces(line.substr(1, 6)));
    }
};

// Length of diff fields in the input file
long long addressLen = 6;
long long labelLen = 10;
long long opcodeLen = 10;
long long operandLen = 30;

class Instruction_2
{
public:
    // Struct to hold intermediate instructions
    string address, label, opcode, operand;
    long long type;
    bool empty;
    Instruction_2(string &line)
    {
        if (line.size() == 0)
        { // Checks if empty line
            empty = true;
            return;
        }
        address = line.substr(0, addressLen);
        address = trimWhitespaces(address);
        line = line.substr(addressLen);
        label = line.substr(0, labelLen);
        label = trimWhitespaces(label); // Parse input line to initialize various variables
        opcode = line.substr(labelLen, opcodeLen);
        opcode = trimWhitespaces(opcode);
        operand = line.substr(labelLen + opcodeLen, operandLen);
        operand = trimWhitespaces(operand);

        if (opcode[0] == '+')
        { // Check extended format instruction
            opcode = opcode.substr(1);
        }
        empty = false;
    }
};

// Addresses of constants from Intermediate file
unordered_map<long long, long long> constAddresses;
// Beginning addresses of section
unordered_map<string, long long> begAddresses;

void checkConstants()
{
    ifstream finput("intermediate.txt");
    string curSec;
    while (getline(finput, line))
    {
        // Read line-by-line and set beginning addresses with length in map
        if (line.size() == 0)
            break;
        if (line[0] == '.')
        { // It is a comment, so we ignore it
            continue;
        }
        Instruction_2 inst(line);
        if (inst.opcode == "START" || inst.opcode == "CSECT")
            curSec = inst.label;
        else if (inst.opcode == "WORD")
            constAddresses[hexToInteger(inst.address) + begAddresses[curSec]] = 3;
        else if (inst.opcode == "BYTE")
            constAddresses[hexToInteger(inst.address) + begAddresses[curSec]] = 1;
        else if (inst.label == "*")
            constAddresses[hexToInteger(inst.address) + begAddresses[curSec]] = inst.operand.size() / 2;
    }
}

void writeToStream(ofstream &fop, char mmap[])
{ // Function to write the mmap in the output file

    checkConstants();
    long long j = 0;

    while (j < textRecords.size())
    {
        auto textRecord = textRecords[j];
        long long address = textRecord.first;
        long long length = textRecord.second;
        long long endadd = address + textRecord.second; // Calculating the end address from start address and length
        char buf[length + 1];                           // Buf for storing string
        strncpy(buf, mmap + address, length);
        buf[length] = '\0';
        string s;
        s = buf;
        long long i = 0;
        while (i < s.size())
        { // Iterating byte-by-byte
            long long curAddress = (i + address) / 2;
            long long len;
            if (constAddresses.count(curAddress))
            { // Checking if constant or Instruction
                len = constAddresses[curAddress];
            }
            else
            {
                long long fb = hexToInteger(s.substr(i, 2));
                string temp = padZeroesToLeft(integerToHex((fb >> 2) << 2), 2);
                len = stoll(LENS[temp]); // Getting length from map LENS
                if (len == 3)
                {
                    long long flags = hexToInteger(string(1, s.substr(i, 3)[2]));
                    if (flags & 1)
                        len = 4;
                }
            }
            fop << s.substr(i, len * 2);
            i += (len * 2);
            if (j + 1 < textRecords.size() || i < s.size())
                fop << endl;
        }
        j++;
    }
}

int main(int argc, char **argv)
{
    initialize_LENS();

    inputFile = "loaderInput.txt";
    outputFile = "loaderOutput.txt";

    ifstream finput(inputFile); // Creating i/o streams for input/output files
    ofstream fop(outputFile);   // Creating i/o streams for input/output files

    /**************** PASS 1 STARTS ****************/
    csaddr = 0;
    progaddr = 0;

    while (getline(finput, line))
    {
        HeaderRecord header(line); // Parse and hold Header line
        cslth = header.length;
        if (ESTAB.count(header.secName))
        { // checking if secName in ESTAB
            cout << "ERROR occurred, ABORTING!" << endl;
            error = true;
        }
        else
        {
            ESTAB[header.secName] = csaddr;
        }

        getline(finput, line);
        while (line[0] != 'E')
        {
            if (line[0] == 'D')
            {
                DefineRecord defRecord(line); // Parsing and storing define record
                for (auto u : defRecord.defSyms)
                {
                    if (ESTAB.count(u.first))
                    { // Checking if symbol already in ESTAB
                        cout << "ERROR occured, ABORTING!" << endl;
                        error = true;
                    }
                    else
                    {
                        ESTAB[u.first] = u.second + csaddr;
                    }
                }
            }
            getline(finput, line); // Reading next line
        }

        if (!finput.eof())
        {
            getline(finput, line);
        }

        csaddr += cslth; // Adding section length to csaddr
    }
    /**************** PASS 1 ENDS ****************/

    finput.clear();
    finput.seekg(0); // Go back to beginning of input file

    endaddr = (csaddr * 2 + 31) / 32 * 32; // Calcuting end address for output file
    char mmap[endaddr];                    // mmap is the helper array which is used to load the program instead of actual memory
    for (long long i = 0; i < endaddr; i++)
        mmap[i] = '.'; // Initializing mmap

    /**************** PASS 2 STARTS ****************/
    execaddr = csaddr = progaddr;
    while (getline(finput, line))
    {
        // Reading line from input file
        HeaderRecord header(line);
        cslth = header.length;
        begAddresses[header.secName] = csaddr;
        getline(finput, line);
        while (line[0] != 'E')
        {
            if (line[0] == 'T')
            {
                TextRecord tRecord(line); // Parsing and storing text record
                tRecord.loadToMem(mmap);  // Load the object code in mmap
            }
            else if (line[0] == 'M')
            {
                ModificationRecord mRecord(line);
                if (ESTAB.count(mRecord.extSym))
                {                            // Is ESTAB has the extsymbol
                    mRecord.updateVal(mmap); // Update the value at the adress
                }
                else
                {
                    cout << "ERROR occurred, ABORTING!" << endl;
                    error = true;
                }
            }
            getline(finput, line); // Read next line
        }
        EndRecord eRecord(line);
        if (eRecord.startingAddress)
        {
            execaddr = csaddr + eRecord.startingAddress; // If starting Address is given, set execaddr to it
        }
        if (!finput.eof())
        {
            getline(finput, line);
        }

        csaddr += cslth; // Adding section length to csaddr
    }
    finput.close();
    /**************** PASS 2 ENDS ****************/

    writeToStream(fop, mmap);
    fop.close();
    if (error)
    {
        cout << "ERROR" << endl;
    }
    else
    {
        cout << "See file " << outputFile << " for the output" << endl;
    }

    return 0;
}