#include <bits/stdc++.h>

using namespace std;

// Prototype for functions declarations
string getObjectCodeOfString(string &operand);

// Data structure declarations
map<string, string> OPTAB;
map<string, string> REGS;
set<string> assembler_directives;

void initialize_OPTAB()
{
    // initialize OPTAB with pairs {mnemonic opcode, machine language equivalent}
    OPTAB["ADD"] = "18";
    OPTAB["COMP"] = "28";
    OPTAB["COMPR"] = "A0";
    OPTAB["DIV"] = "24";
    OPTAB["J"] = "3C";
    OPTAB["JEQ"] = "30";
    OPTAB["JGT"] = "34";
    OPTAB["JLT"] = "38";
    OPTAB["JSUB"] = "48";
    OPTAB["LDA"] = "00";
    OPTAB["LDB"] = "68";
    OPTAB["LDT"] = "74";
    OPTAB["LDCH"] = "50";
    OPTAB["LDL"] = "08";
    OPTAB["LDX"] = "04";
    OPTAB["MUL"] = "20";
    OPTAB["RD"] = "D8";
    OPTAB["RSUB"] = "4C";
    OPTAB["STA"] = "0C";
    OPTAB["STCH"] = "54";
    OPTAB["STL"] = "14";
    OPTAB["STX"] = "10";
    OPTAB["SUB"] = "1C";
    OPTAB["TD"] = "E0";
    OPTAB["TIX"] = "2C";
    OPTAB["TIXR"] = "B8";
    OPTAB["WD"] = "DC";
    OPTAB["CLEAR"] = "B4";
}

void initialize_REGS()
{
    // REGS maps register name to number
    REGS["A"] = "0";
    REGS["X"] = "1";
    REGS["L"] = "2";
    REGS["B"] = "3";
    REGS["S"] = "4";
    REGS["T"] = "5";
    REGS["F"] = "6";
    REGS["PC"] = "8";
    REGS["SW"] = "9";
}

void initialize_assembler_directives()
{
    // assembler directives
    assembler_directives.insert("START");
    assembler_directives.insert("END");
    assembler_directives.insert("BYTE");
    assembler_directives.insert("WORD");
    assembler_directives.insert("RESB");
    assembler_directives.insert("RESW");
    assembler_directives.insert("BASE");
    assembler_directives.insert("EQU");
    assembler_directives.insert("LTORG");
    assembler_directives.insert("EXTDEF");
    assembler_directives.insert("EXTREF");
}

void makeStringEven(string &s)
{
    // pad string with 0 if necessary to make it even length
    if (s.size() == 0)
    {
        s = "0";
    }
    int n = s.size() - 1;
    if (n % 2 == 0)
    {
        s = "0" + s;
    }
}

// helper function to convert HEX to integer
long long hexToInteger(string &hexstr)
{
    long long num = 0;
    num = stoi(hexstr, 0, 16);
    return num;
}

// helper function to convert integer to HEX
string integerToHex(long long n)
{
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

// important variable declarations
long long LOCCTR;
long long base;
string line;
string subRoutineName;
string inputFileName, interFileName, outputFileName;
long long passNum;
string firstSubroutine, lastSubroutine;

struct LitInfo
{
    // struct to handle literals
    string value, address;
    long long length;
    bool isString;
    LitInfo(string litName)
    {
        isString = true;
        if (litName == "*")
        {
            // if litName="*", then set value to be LOCCTR in HEX
            string s = integerToHex(LOCCTR);
            makeStringEven(s);
            value = s;
        }
        else
            value = getObjectCodeOfString(litName); // otherwise get obejct code
        makeStringEven(value);
        length = value.size() / 2;
    }
    LitInfo() {}
};

set<string> globalSymtab;                 // globalsymtab for extdefs
map<string, map<string, string>> symtabs; // Map for symtabs of sections
map<string, LitInfo> LITTAB;
map<string, long long> length, startingAddresses; // storing lengths and starting addresses of sections

unordered_set<string> extReferences; // set for managing external references

void setBit(string &hexStr, long long pos)
{
    // set bit in hex string at pos
    long long tmp = (hexToInteger(hexStr) | (1 << pos));
    hexStr = integerToHex(tmp);
}

string padZeroesToLeft(string s, long long sz = 6)
{
    // Pads string with zeroes in the beginning
    string temp;
    for (long long i = 0; i < (sz - (long long)s.size()); i++)
        temp += '0';
    return temp + s;
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

void trimWhitespaces(string &s)
{
    const char *t = " \t\n\r\f\v";
    string tmp = ltrim(rtrim(s, t), t);
    s = tmp;
}

void removeTrailingWhitespaces(string &s)
{
    // remove trailing whitepaces
    const char *t = " \t\n\r\f\v";
    string tmp = rtrim(s, t);
    s = t;
}

struct ModificationRecord
{
    // struct to handle Modification Records
    vector<string> records; // List of mrecords
    void addRecord(string &address, long long length, char sign, string &name)
    { // Adding record to list
        stringstream ss;
        ss << "M^"
           << right << setfill('0') << setw(6) << address << "^"
           << right << setfill('0') << setw(2) << uppercase << hex << length << "^"
           << sign
           << left << setfill(' ') << setw(6) << name;
        records.push_back(ss.str());
    }
    void writeToStream(ofstream &fop)
    { // Writing records to out stream
        for (string record : records)
            fop << record << '\n';
        records.clear();
    }
};

map<string, ModificationRecord> mRecords; // Map to keep track of mrecords for diff sections

long long findType(string &operand)
{
    // Find type of inst,whether 1 or 2 or 3 or 4
    if (operand.size() == 0)
    {
        return 1;
    }
    string lef = "";
    if (operand.find(",") == string::npos)
    {
        lef = operand;
    }
    else
    {
        lef = operand.substr(0, operand.find(","));
    }
    if (REGS.count(lef))
        return 2;
    return 3;
}

// Error flag
bool error = false;

void handleAddressType2(string &operand)
{
    // Helper function to get object codes of
    long long n = operand.size(); // instruction of type 2
    long long mi = n;
    for (long long i = 0; i < n; i++)
    {
        if (operand[i] == ',')
        {
            mi = i;
            break;
        }
    }
    // Get register name from operand
    string leftReg = "";
    if (operand.find(",") == string::npos)
    {
        leftReg = operand;
    }
    else
    {
        leftReg = operand.substr(0, operand.find(","));
    }
    if (REGS.count(leftReg) == 0)
    {
        // If register not valid, set errorflag
        cout << "ERROR , ABORTING!";
        error = true;
        return;
    }
    if (mi == n)
    {
        operand = REGS[leftReg] + "0";
    }
    else
    {
        string rightReg = operand.substr(mi + 1); // Get register name from operand
        if (REGS.count(rightReg) == 0)
        {
            cout << "ERROR , ABORTING!";
            error = true;
            return;
        }
        operand = REGS[leftReg] + REGS[rightReg];
    }
}

void getAbsoluteAddress(string &operand)
{ // Find value of operand
    if (symtabs[subRoutineName].count(operand))
    {
        operand = symtabs[subRoutineName][operand]; // If found symtab, return value
    }
    else if (operand[0] == '=')
    {
        string litName = operand.substr(1);
        // If literal, get value from LITTAB
        if (litName == "*")
        {
            // store hex value of LOCCTR in litName
            string s = integerToHex(LOCCTR);
            makeStringEven(s);
            litName = s;
        }
        operand = LITTAB[litName].address;
    }
    else
    {
        try
        {
            operand = integerToHex(stoi(operand)); // If immediate no, get hex value
        }
        catch (const exception &e)
        {
            cout << "ERROR , ABORTING!";
            error = true;
        }
    }
}

string getHex2Comp(long long num)
{ // get num in 2s complement form
    if (num >= 0)
    {
        return integerToHex(num);
    }
    else
    {
        // Calculating 2s complement if num<0
        return integerToHex((1 << 12) + num);
    }
}

void handleAddressType3(string &operand)
{                                // Helper function to get object codes of
    getAbsoluteAddress(operand); // instruction of type 3

    long long address = hexToInteger(operand);
    long long pc = LOCCTR + 3;
    if (address - pc >= -(1 << 11) && address - pc <= ((1 << 11) - 1))
    { // Checks if PC relative addressing works
        operand = getHex2Comp(address - pc);
        setBit(operand, 13);
    }
    else if (address - base >= -(1 << 11) && address - base <= ((1 << 11) - 1))
    { // Checks if base relative addressing works
        operand = getHex2Comp(address - base);
        setBit(operand, 14);
    }
    else
    { // Otherwise set error flag
        cout << "ERROR , ABORTING!";
        error = true;
    }
}

void handleAddressType4(string &operand, bool isImmediate, string address)
{ // Helper function to get object codes of
    if (!isImmediate)
    { // instruction of type 4
        if (extReferences.count(operand))
        {
            mRecords[subRoutineName].addRecord(address, 5, '+', operand); // Add mrecord if Address type is 4
        }
        else
        {
            mRecords[subRoutineName].addRecord(address, 5, '+', subRoutineName);
        }
    }
    getAbsoluteAddress(operand);
    setBit(operand, 20); // Set bit 20
}

bool isLiteral(string &s)
{ // checks if string is literal
    if (s.size() > 0 && s[0] == '=')
        return true;
    return false;
}

string getObjectCodeOfString(string &operand)
{ // returns object code of operand
    string res = "";
    long long n = (operand).size();
    if (operand[0] == 'C')
    {
        // character string
        // loop for actual string inside quotes i.e. from pos 2..(n-2)
        for (long long i = 2; i <= n - 2; i++)
        {
            string tmp = integerToHex((long long)(operand[i]));
            if (tmp.size() == 1)
            {
                res += '0';
            }
            res += tmp;
        }
    }
    else
    {
        // HEX string, can take substring directly of part inside quotes
        res += (operand).substr(2, n - 3);
    }
    return res;
}

// Length of diff fields in the input file
long long addressLen = 6;
long long labelLen = 10;
long long opcodeLen = 10;
long long operandLen = 30;

// class for input instructions
class Instruction_1
{
public:
    string label, opcode, operand;
    long long type, length;
    bool valid;
    Instruction_1(string &line)
    {
        valid = true;
        if (line.size() == 0)
        { // Checks if invalid line
            valid = false;
            return;
        }
        label = "";
        opcode = "";
        operand = "";
        initInstruction(line);
        setInstLength();
    }

    void initInstruction(string &line)
    {
        // instruction is in the given format of label,opcode,operand
        this->label = line.substr(0, labelLen);
        this->opcode = line.substr(labelLen, opcodeLen);
        this->operand = line.substr(labelLen + opcodeLen, operandLen);
        trimWhitespaces(opcode);
        trimWhitespaces(label);
        trimWhitespaces(operand);

        if (this->opcode[0] == '+')
        {
            // Check extended format instruction
            this->type = 4;
            this->opcode = (this->opcode).substr(1, this->opcode.size() - 1);
        }
        else if (this->opcode == "RSUB")
            this->type = 3;
        else
        {
            this->type = findType(this->operand); // Call findType to get type of instruction from operand
        }
        valid = true;
    }

    bool isLabel()
    { // returns true if there is label in the instruction
        return (label.size() > 0);
    }
    long long getIntOperand()
    {
        // Converts operand to long long
        return stoll(operand);
    }
    long long getLengthByteArg()
    { // returns length of string in BYTE operand
        long long n = operand.size();
        if (operand[0] == 'C')
        { // Checks if char or hex string
            return n - 3;
        }
        else
        {
            return (n - 3) / 2;
        }
    }
    bool operandIsLiteral()
    { // Checks if the operand is a literal
        return isLiteral(operand);
    }
    void setInstLength()
    { // set the length of instruction
        length = type;
    }
};

// class intermediate instructions
class Instruction_2
{
public:
    string address, label, opcode, operand;
    long long type;
    bool valid;
    Instruction_2(string &line)
    {
        valid = true;
        if (line.size() == 0)
        { // Checks if invalid line
            valid = false;
            return;
        }
        address = line.substr(0, addressLen);
        trimWhitespaces(address);
        string remainLine = line.substr(addressLen);
        initInstruction(remainLine);
    }

    void initInstruction(string &line)
    {
        // instruction is in the given format of label,opcode,operand
        this->label = line.substr(0, labelLen);
        this->opcode = line.substr(labelLen, opcodeLen);
        this->operand = line.substr(labelLen + opcodeLen, operandLen);
        trimWhitespaces(opcode);
        trimWhitespaces(label);
        trimWhitespaces(operand);

        if (this->opcode[0] == '+')
        {
            // Check extended format instruction
            this->type = 4;
            this->opcode = (this->opcode).substr(1, this->opcode.size() - 1);
        }
        else if (this->opcode == "RSUB")
            this->type = 3;
        else
        {
            this->type = findType(this->operand); // Call findType to get type of instruction from operand
        }
        valid = true;
    }

    string getObjectCodeByteOperand()
    { // get Object code in hex from BYTE string operand
        return getObjectCodeOfString(operand);
    }
    bool operandIsLiteral()
    { // Checks if operand is a literal
        return isLiteral(operand);
    }
    void calculateAddressOC()
    { // Calculates Object code of the operand
        if (type == 1)
        {
            return;
        }
        else if (type == 2)
        {
            handleAddressType2(operand); // Calling appropriate helper fn for diff types
        }
        else
        {
            if (opcode == "RSUB")
            { // Handle RSUB seperately
                operand = "0";
                setBit(operand, 16);
                setBit(operand, 17);
                return;
            }
            bool isIndirect = false, isImmediate = false, idx = false;
            if (operand[0] == '#')
                isImmediate = true; // Checking for immediate and indirect addressing
            if (operand[0] == '@')
                isIndirect = true;
            long long ci = operand.size();
            for (long long i = 0; i < ((long long)operand.size()); i++)
            { // Checks for indexed addressing
                if (operand[i] == ',')
                {
                    ci = i;
                    idx = true;
                    break;
                }
            }
            operand.erase(ci);
            if (isImmediate || isIndirect)
                operand = operand.substr(1);
            long long c;
            if (type == 3)
            {
                c = 12;
                if (!isImmediate)
                    handleAddressType3(operand); // Handles type 3 addresses
            }
            else
            {
                c = 20;
                bool isExtRef = extReferences.count(operand);
                handleAddressType4(operand, isImmediate, integerToHex(hexToInteger(address) + 1)); // Handles type 4 addresses
            }
            if (idx)
                setBit(operand, c + 3); // Set bits in object code accordingly
            if (isImmediate || !isIndirect)
                setBit(operand, c + 4);
            if (isIndirect || !isImmediate)
                setBit(operand, c + 5);
            // if(!isImmediate && !isIndirect) {setBit(operand, c+4); setBit(operand, c+5);}
        }
    }
    string getObjectCode()
    { // Gives object code for entire instruction
        calculateAddressOC();
        string opCodeV = OPTAB[opcode];
        string objectCode;
        if (type == 1)
        {
            objectCode = opcode;
        }
        else if (type == 2)
        {
            objectCode = opCodeV + operand;
        }
        else if (type == 3)
        {
            long operandL = hexToInteger(operand);
            objectCode = integerToHex((hexToInteger(opCodeV) << 16) + operandL); // Shift the opcode to appropriate position in instruction OC
        }
        else if (type == 4)
        {
            long operandL = hexToInteger(operand);
            objectCode = integerToHex((hexToInteger(opCodeV) << 24) + operandL); // Shift the opcode to appropriate position in instruction OC
        }
        objectCode = padZeroesToLeft(objectCode, type * 2);
        return objectCode;
    }
};

class TextRecord
{
public:
    // class for representing text records
    vector<string> objectCodes;
    long long limit;
    long long size;
    string startingAddress;

    // constructor
    TextRecord()
    {
        this->limit = 30;
        this->size = 0;
    }

    void addObjectCode(ofstream &fop, string objectCode, string address)
    { // Adds object code to the text record
        if (!objectCodes.size())
            startingAddress = address;
        if (size + objectCode.size() / 2 > limit)
        { // If size exceeds limit write to file
            writeRecordOnStream(fop);
            addObjectCode(fop, objectCode, address);
        }
        else
        {
            objectCodes.push_back(objectCode);
            size += (objectCode.size() / 2);
        }
    }

    void writeRecordOnStream(ofstream &fop)
    { // Writes record to output file stream
        if (objectCodes.size() == 0)
            return;

        fop << "T^" // Writing the beginning of Text record
            << right << setfill('0') << setw(6) << startingAddress << "^"
            << right << setfill('0') << setw(2) << uppercase << hex << size;

        for (string &x : objectCodes)
        {
            // Writing object codes
            if (x.size())
            {
                fop << "^" << x;
            }
        }
        fop << '\n';
        objectCodes.clear();
        size = 0; // Clearing for next record
        startingAddress = "";
    }
};

// create an instance of class TextRecord
TextRecord textRecord;

void addressSymbolToValue(string &operand)
{ // get address opcode from instruction operand
    long long ci = operand.size();
    if (ci == 0)
    { // If no operand, TYPE 1
        return;
    }
    bool directAddressing = true;
    for (long long i = 0; i < ((long long)operand.size()); i++)
    {
        if (operand[i] == ',')
        {
            ci = i;
            directAddressing = false;
            break;
        }
    }
    operand.erase(ci);
    if (symtabs[subRoutineName].count(operand))
    { // Checks if operand in symtabs[programName]
        operand = symtabs[subRoutineName][operand];
    }
    else
    { // else but operand as 0 and error=true
        operand = "0000";
        cout << "ERROR , ABORTING!";
        error = true;
        return;
    }
    if (!directAddressing)
    { // Indexed addressing
        setBit(operand, 15);
    }
    operand = padZeroesToLeft(operand, 4);
}

void writeHeaderRecord(ofstream &fop)
{ // Write HEADER record to output file stream
    fop << "H^" << left << setfill(' ') << setw(6) << subRoutineName << "^";
    fop << right << setfill('0') << setw(6) << uppercase << hex << startingAddresses[subRoutineName] << "^";
    fop << setfill('0') << setw(6) << uppercase << hex << length[subRoutineName] << '\n';
}

void writeEndRecord(ofstream &fop)
{ // Write END record to output file stream
    fop << "E";
    if (subRoutineName == firstSubroutine)
        fop << "^" << right << setfill('0') << setw(6) << uppercase << hex << startingAddresses[subRoutineName];
    if (subRoutineName != lastSubroutine)
        fop << "\n\n";
}

void writeLineToStream(ofstream &fop)
{ // Write current line to output file stream
    fop << right << setfill('0') << setw(addressLen - 1) << uppercase << hex << LOCCTR;
    fop << " " << line << '\n';
}

void writeStringToStream(ofstream &fop, string &objectCode, Instruction_2 &inst)
{
    int n = (int)objectCode.size();
    for (int i = 0; (i + 6) <= n; i += 6)
    {
        // 2 columns = 1 byte of object code
        // we take blocks of size of 3 bytes = 1 word at a time
        string curr = integerToHex(hexToInteger(inst.address) + i);
        string curBlock = "";
        for (int j = i; j < i + 6; j++)
        {
            curBlock += objectCode[j];
        }
        textRecord.addObjectCode(fop, curBlock, curr);
    }
    // also add the remaining instructions, if any left
    string rem = "";
    for (int i = (n / 6) * 6; i < n; i++)
    {
        rem += objectCode[i];
    }
    if (n > ((n / 6) * 6))
    {
        textRecord.addObjectCode(fop, rem, inst.address);
    }
}

void handleStrings(ofstream &fop, Instruction_2 &inst)
{
    // Handles character and hex BYTE strings
    string objectCode = inst.getObjectCodeByteOperand();
    writeStringToStream(fop, objectCode, inst);
}

string findValue(string &expression, string address = "")
{
    // Find value of expression
    string temp;
    long val = 0;
    char lastSym = '+'; // Doing initializations
    expression += '+';
    for (long long i = 0; i < (int)(expression.size()); i++)
    { // Iterating through the expression
        if (expression[i] == '-' || expression[i] == '+' || expression[i] == '/' || expression[i] == '*')
        {
            long num;
            if (symtabs[subRoutineName].find(temp) != symtabs[subRoutineName].end())
                num = hexToInteger(symtabs[subRoutineName][temp]);
            else
                num = stol(temp);
            if (lastSym == '-')
                val -= num; // Handling the signs
            else if (lastSym == '+')
                val += num;
            else if (lastSym == '/')
                val /= num;
            else if (lastSym == '*')
                val *= num;
            if (extReferences.count(temp) > 0 && passNum == 2)
            {
                // If its a ext ref, add mrecord
                mRecords[subRoutineName].addRecord(address, 6, lastSym, temp);
            }
            lastSym = expression[i];
            temp = "";
        }
        else
            temp += expression[i];
    }
    string retStr = integerToHex(val);
    // return the final value as hex
    return retStr;
}

struct ReferenceRecord
{
    // Struct to store reference record
    vector<string> names;
    void writeRecordOnStream(ofstream &fop)
    {
        // Writes record to output file stream
        if (names.size() == 0)
            return;
        long long sz = names.size();
        for (long long i = 0; i < sz; i += 12)
        {
            fop << "R^";
            int lastIndex = min(sz, i + 12);
            for (long long j = i; j < lastIndex; j++)
            {
                if (j == lastIndex - 1)
                {
                    fop << left << setfill(' ') << setw(6) << names[j];
                    continue;
                }
                fop << left << setfill(' ') << setw(6) << names[j] << "^";
            }
            fop << '\n';
        }
        names.clear();
    }

    void checkExtref()
    { // Checks if extrefs are actually present in the globalsymtab
        for (string name : names)
        {
            if (globalSymtab.count(name) == 0)
            {
                cout << "ERROR , ABORTING!";
                error = true;
            }
        }
    }
};

map<string, ReferenceRecord> rRecords; // Map to hold ReferenceRecord for each section

struct DefineRecord
{
    // Struct to store Define record
    vector<string> names;
    vector<string> addresses;
    void writeRecordOnStream(ofstream &fop)
    { // Writes record to output file stream
        if (names.size() == 0)
            return;
        long long sz = names.size();
        for (long long i = 0; i < sz; i += 6)
        {
            fop << "D^";
            int lastIndex = min(sz, i + 6);
            for (long long j = i; j < min(sz, i + 6); j++)
            {
                if (j == lastIndex - 1)
                {
                    fop << left << setfill(' ') << setw(6) << names[j] << "^"
                        << right << setfill('0') << setw(6) << addresses[j];
                    continue;
                }
                fop << left << setfill(' ') << setw(6) << names[j] << "^"
                    << right << setfill('0') << setw(6) << addresses[j] << "^";
            }
            fop << '\n';
        }
        names.clear(); // Clearing names and addresses
        addresses.clear();
    }

    void addRecord(string &name)
    { // Add record to name
        names.push_back(name);
    }
};

map<string, DefineRecord> dRecords; // Map to hold DefineRecord for each section

void writeLitToStream(ofstream &fop, LitInfo &litInfo, string name)
{
    // Writes literal to intermediary file
    fop << right << setfill('0') << setw(addressLen - 1) << uppercase << hex << LOCCTR << ' ';
    fop << left << setfill(' ') << setw(labelLen) << "*";
    fop << left << setfill(' ') << setw(opcodeLen) << "";
    fop << left << setfill(' ') << setw(operandLen) << litInfo.value;
    fop << "\n";
}

void handleExtdef(string &operands)
{
    // Handles external defines
    string temp;
    operands += ',';
    DefineRecord drecord;
    for (long long i = 0; i < operands.size(); i++)
    {
        if (operands[i] != ',')
            temp += operands[i];
        else
        {
            globalSymtab.insert(temp); // Inserts in global symtab
            drecord.addRecord(temp);   // Add to drecord
            temp = "";
        }
    }
    dRecords[subRoutineName] = drecord;
}

void handleExtref(string &operands)
{
    // Handles external defines
    string temp;
    operands += ",";
    ReferenceRecord rRecord;
    for (long long i = 0; i < (int)operands.size(); i++)
    {
        if (operands[i] != ',')
            temp += operands[i];
        else if (temp != "")
        {
            extReferences.insert(temp);
            // Inserts in appropriate data structures rRecord and also in symtabs
            rRecord.names.push_back(temp);
            symtabs[subRoutineName][temp] = "0";
            temp = "";
        }
    }
    rRecords[subRoutineName] = rRecord; // Inserting ReferenceRecord in the map
}

void addExtref(string &operands)
{
    // Adds external references to extrefs DS
    string temp;
    operands += ",";
    for (long long i = 0; i < (int)operands.size(); i++)
    {
        if (operands[i] != ',')
            temp += operands[i];
        else if (temp != "")
        {
            extReferences.insert(temp);
            temp = "";
        }
    }
}

void writeRecordsOnStream(ofstream &fop)
{
    textRecord.writeRecordOnStream(fop);         // Write the last text record in output file
    mRecords[subRoutineName].writeToStream(fop); // Write M records
    writeEndRecord(fop);
}

void writeLitsToStream(ofstream &fop)
{
    // Write lits the output file in the proper locations
    for (auto &it : LITTAB)
    {
        LitInfo &litInfo = LITTAB[it.first];
        if (litInfo.address.size() == 0)
        {
            litInfo.address = integerToHex(LOCCTR); // Calculates address for literals
            writeLitToStream(fop, litInfo, it.first);
            LOCCTR += litInfo.length; // Updates LOCCTR
        }
    }
}

int main(int argc, char **argv)
{
    initialize_assembler_directives();
    initialize_OPTAB();
    initialize_REGS();

    if (argc != 2)
    {
        cout << "Invalid input, try again!" << endl;
        return -1;
    }

    inputFileName = argv[1];
    interFileName = "intermediate.txt";
    outputFileName = "assemblerOutput.txt";
    ifstream finput(inputFileName); // Creating i/o streams for input/output files
    ofstream interOp(interFileName);

    /**************** PASS 1 STARTS ****************/
    passNum = 1;
    while (getline(finput, line))
    {
        if (line.size() == 0)
            break;
        if (line[0] == '.')
        {
            // It is a comment
            continue;
        }
        Instruction_1 inst(line);
        if (!inst.valid)
            continue;
        // start of a control section / end of the program
        if (inst.opcode == "CSECT" || inst.opcode == "END")
        {
            // After finishing section, we come here
            writeLitsToStream(interOp);
            length[subRoutineName] = LOCCTR - startingAddresses[subRoutineName];
            for (auto u : dRecords[subRoutineName].names)
            { // Loop through dRecords
                if (symtabs[subRoutineName].count(u) > 0)
                { // Set addresses for the Names in drecords
                    dRecords[subRoutineName].addresses.push_back(symtabs[subRoutineName][u]);
                }
                else
                {
                    cout << "ERROR , ABORTING!";
                    error = true;
                }
            }
        }
        if (inst.opcode == "START" || inst.opcode == "CSECT")
        { // If START/CSECT inst, initializations done
            subRoutineName = inst.label;
            lastSubroutine = subRoutineName;
            if (inst.opcode == "START")
            {
                startingAddresses[subRoutineName] = hexToInteger(inst.operand); // Setting startAddress
                firstSubroutine = subRoutineName;                               // Setting name of 1st subroutine
            }
            else
                startingAddresses[subRoutineName] = 0;
            LOCCTR = startingAddresses[subRoutineName];
            globalSymtab.insert(subRoutineName); // Insert name in globalsymtab
            mRecords[subRoutineName] = ModificationRecord();
            writeLineToStream(interOp); // Write line to intermediate file
            continue;
        }
        else
        {
            writeLineToStream(interOp); // Write line to intermediate file
            if (inst.opcode == "END")
            { // If END, write size in intermediate file
                break;
                // interOp << LOCCTR - startingAddresses[subRoutineName] << '\n';
            }
            if (inst.isLabel())
            {
                if (symtabs[subRoutineName].count(inst.label))
                { // Duplicate label
                    cout << "ERROR , ABORTING!";
                    error = true;
                }
                else
                {
                    symtabs[subRoutineName][inst.label] = integerToHex(LOCCTR); // Inserting (Label, LOCCTR) in symtab
                }
            }
            if (inst.operandIsLiteral())
            { // Handle literals
                string litName = inst.operand.substr(1);
                LitInfo litInfo(litName);
                if (litName == "*")
                { // Handle =* literal specially
                    litName = litInfo.value;
                }
                if (!LITTAB.count(litName))
                    LITTAB[litName] = litInfo; // Insert symbol in LITTAB
            }
            if (OPTAB.count(inst.opcode))
            {
                LOCCTR += inst.length;
            }
            else if (inst.opcode == "LTORG")
            {
                writeLitsToStream(interOp); // If LTORG, calc address andwrite lits to stream
            }
            else if (inst.opcode == "WORD")
            {
                LOCCTR += 3; // LOCCTR jumps 1 word ie 3 bytes
            }
            else if (inst.opcode == "RESW")
            {
                LOCCTR += (3 * inst.getIntOperand()); // LOCCTR jumps 3*noOfWords bytes
            }
            else if (inst.opcode == "RESB")
            {
                LOCCTR += inst.getIntOperand();
            }
            else if (inst.opcode == "BYTE")
            { // LOCCTR jumps size of string
                LOCCTR += inst.getLengthByteArg();
            }
            else if (inst.opcode == "EQU")
            {
                if (inst.operand == "*")
                {
                    // Insert LOCCTR in symtab of corresponding subroutine
                    symtabs[subRoutineName][inst.label] = integerToHex(LOCCTR);
                }
                else
                {
                    // Insert value of expression in symtab
                    symtabs[subRoutineName][inst.label] = findValue(inst.operand);
                }
            }
            else if (inst.opcode == "EXTDEF")
            {
                handleExtdef(inst.operand); // Parses and handles extdef
            }
            else if (inst.opcode == "EXTREF")
            {
                handleExtref(inst.operand); // Parses and handles extref
            }
            else
            {
                cout << "ERROR , ABORTING!";
                error = true;
            }
        }
    }
    finput.close();
    interOp.close();

    /**************** PASS 1 ENDS ****************/

    /**************** PASS 2 STARTS ****************/
    ifstream interFile(interFileName);
    ofstream fop(outputFileName);
    passNum = 2;

    while (getline(interFile, line))
    {
        if (line.size() == 0)
            break;
        if (line[0] == '.')
        { // It is a comment, so we ignore it
            // interOp << line << '\n';
            continue;
        }
        Instruction_2 inst(line); // Converting line to Instruction_2 struct
        LOCCTR = hexToInteger(inst.address);
        if (!inst.valid)
            continue;

        string objectCode;
        if (inst.opcode == "END")
        {
            writeRecordsOnStream(fop);
            break;
        }

        if (OPTAB.count(inst.opcode))
        { // Check if opcode in OPTAB

            objectCode = inst.getObjectCode();
        }
        else if (inst.opcode == "WORD")
        {
            objectCode = padZeroesToLeft(findValue(inst.operand, inst.address)); // Convert Int to HEX
        }
        else
        {
            if (inst.opcode == "START" || inst.opcode == "CSECT")
            {
                if (inst.opcode == "CSECT")
                {
                    writeRecordsOnStream(fop);
                }
                extReferences.clear();
                subRoutineName = inst.label;
                writeHeaderRecord(fop); // Write HEADER record
            }
            else if (inst.opcode == "BYTE")
            {
                handleStrings(fop, inst); // Handles character and hex BYTE strings
            }
            else if (inst.opcode == "BASE")
            {
                base = hexToInteger(symtabs[subRoutineName][inst.operand]);
            }
            else if (inst.opcode == "RESW")
            {
                textRecord.writeRecordOnStream(fop); // Writes last record because there is gap
            }
            else if (inst.opcode == "RESB")
            {
                textRecord.writeRecordOnStream(fop); // Writes last record because there is gap
            }
            else if (inst.opcode == "EXTDEF")
            {
                dRecords[subRoutineName].writeRecordOnStream(fop); // write DRecords on stream
            }
            else if (inst.opcode == "EXTREF")
            {
                // Check if valid references
                rRecords[subRoutineName].checkExtref();
                rRecords[subRoutineName].writeRecordOnStream(fop); // write RRecords on stream
                addExtref(inst.operand);
            }
            else if (inst.label == "*")
            { // Handle Literals
                writeStringToStream(fop, inst.operand, inst);
            }
            continue;
        }
        textRecord.addObjectCode(fop, objectCode, inst.address);
    }

    fop.close();
    interFile.close();

    /**************** PASS 2 ENDS ****************/

    if (error)
    {
        cout << "INVALID Input\n";
    }
    else
    {
        cout << "See file " << outputFileName << " for the machine code\n";
    }

    return 0;
}