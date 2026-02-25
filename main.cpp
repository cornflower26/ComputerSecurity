#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>

using namespace std;

// ─── Permutation Tables copied from chatGPT

// ─── Permuted Choice 1 (PC-1): 64-bit key → 56-bit key (drops parity bits) ───
// Indexed from 1; values indicate which bit of the original 64-bit key to take.
const std::array<int, 56> PC1 = {
        57, 49, 41, 33, 25, 17,  9,
        1, 58, 50, 42, 34, 26, 18,
        10,  2, 59, 51, 43, 35, 27,
        19, 11,  3, 60, 52, 44, 36,
        63, 55, 47, 39, 31, 23, 15,
        7, 62, 54, 46, 38, 30, 22,
        14,  6, 61, 53, 45, 37, 29,
        21, 13,  5, 28, 20, 12,  4
};

// ─── Permuted Choice 2 (PC-2): 56-bit key → 48-bit subkey ───────────────────
const std::array<int, 48> PC2 = {
        14, 17, 11, 24,  1,  5,
        3, 28, 15,  6, 21, 10,
        23, 19, 12,  4, 26,  8,
        16,  7, 27, 20, 13,  2,
        41, 52, 31, 37, 47, 55,
        30, 40, 51, 45, 33, 48,
        44, 49, 39, 56, 34, 53,
        46, 42, 50, 36, 29, 32
};

// ─── Left shift schedule (number of left rotations per round) ────────────────
const std::array<int, 16> SHIFT_SCHEDULE = {
        1, 1, 2, 2, 2, 2, 2, 2,
        1, 2, 2, 2, 2, 2, 2, 1
};

const std::array<int, 64> IP = {
        58, 50, 42, 34, 26, 18, 10,  2,
        60, 52, 44, 36, 28, 20, 12,  4,
        62, 54, 46, 38, 30, 22, 14,  6,
        64, 56, 48, 40, 32, 24, 16,  8,
        57, 49, 41, 33, 25, 17,  9,  1,
        59, 51, 43, 35, 27, 19, 11,  3,
        61, 53, 45, 37, 29, 21, 13,  5,
        63, 55, 47, 39, 31, 23, 15,  7
};

const std::array<int, 64> IP_INV = {
        40,  8, 48, 16, 56, 24, 64, 32,
        39,  7, 47, 15, 55, 23, 63, 31,
        38,  6, 46, 14, 54, 22, 62, 30,
        37,  5, 45, 13, 53, 21, 61, 29,
        36,  4, 44, 12, 52, 20, 60, 28,
        35,  3, 43, 11, 51, 19, 59, 27,
        34,  2, 42, 10, 50, 18, 58, 26,
        33,  1, 41,  9, 49, 17, 57, 25
};

// Expansion table E: 32-bit R → 48 bits
const std::array<int, 48> E = {
        32,  1,  2,  3,  4,  5,
        4,  5,  6,  7,  8,  9,
        8,  9, 10, 11, 12, 13,
        12, 13, 14, 15, 16, 17,
        16, 17, 18, 19, 20, 21,
        20, 21, 22, 23, 24, 25,
        24, 25, 26, 27, 28, 29,
        28, 29, 30, 31, 32,  1
};

// P permutation: applied after S-box substitution
const std::array<int, 32> P = {
        16,  7, 20, 21, 29, 12, 28, 17,
        1, 15, 23, 26,  5, 18, 31, 10,
        2,  8, 24, 14, 32, 27,  3,  9,
        19, 13, 30,  6, 22, 11,  4, 25
};

// S-boxes: 8 boxes, each maps 6 bits → 4 bits
const std::array<std::array<std::array<int, 16>, 4>, 8> SBOXES = {{
                                                                          // S1
                                                                          {{ {14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7},
                                                                             { 0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8},
                                                                             { 4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0},
                                                                             {15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13} }},
                                                                          // S2
                                                                          {{ {15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10},
                                                                             { 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5},
                                                                             { 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15},
                                                                             {13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9} }},
                                                                          // S3
                                                                          {{ {10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8},
                                                                             {13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1},
                                                                             {13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7},
                                                                             { 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12} }},
                                                                          // S4
                                                                          {{ { 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15},
                                                                             {13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9},
                                                                             {10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4},
                                                                             { 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14} }},
                                                                          // S5
                                                                          {{ { 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9},
                                                                             {14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6},
                                                                             { 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14},
                                                                             {11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3} }},
                                                                          // S6
                                                                          {{ {12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11},
                                                                             {10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8},
                                                                             { 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6},
                                                                             { 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13} }},
                                                                          // S7
                                                                          {{ { 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1},
                                                                             {13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6},
                                                                             { 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2},
                                                                             { 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12} }},
                                                                          // S8
                                                                          {{ {13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7},
                                                                             { 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2},
                                                                             { 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8},
                                                                             { 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11} }}
                                                                  }};



std::vector<std::string> readStringsFromFile(const std::string& filename) {
    std::vector<std::string> result;
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        result.push_back(line);

    }

    file.close();
    return result;
}

void possibleSolution(std::string input_file){
    std::vector<std::string> pair = readStringsFromFile(input_file);
    //std::cout << "Ciphertext : " << pair[0] << std::endl;
    //std::cout << "Plaintext : " << pair[1] << std::endl;

    std::vector<std::vector<int>> letters(2,std::vector<int>(26,0));

    //std::cout << "Count :" << std::endl;
    for (int i = 0; i < pair.size(); i++){
        //std::cout << "input " << i << ": ";
        for (int j = 0; j < 26; j++){
            letters[i][j] = int(std::ranges::count(pair[i], char(j+65)));
            //std::cout << char(j+65) << " " << letters[i][j] << ", ";
        }
        //std::cout << std::endl;
    }
    std::sort(letters[0].begin(), letters[0].end());
    std::sort(letters[1].begin(),letters[1].end());

    //std::cout << "Sorted" << std::endl;
    bool flag = true;
    int iterator = 0;
    while (flag && iterator < letters[0].size()){
        if (letters[0][iterator] != letters[1][iterator]){
            flag = false;
            std::cout << "NO" << std::endl;
        }
        iterator++;
    }
    if (flag){
        std::cout << "YES" << std::endl;
    }
}
void printDivisors(int n) {

    for (int i = 1; i <= sqrt(n); i++) {
        if (n % i == 0) {
            if (n / i == i) {
                std::cout << " " << i ;
            }
            else {
                std::cout << " " << i ;
                std::cout << " " << n/i ;
            }
        }
    }
}

double indexOfCoincidence(const std::string& text) {
    std::array<int, 26> freq = {};
    for (char c : text) {
        freq[std::tolower(c) - 'a']++;
    }
    double N = double(text.size());
    double numerator = 0;
    for (int f : freq) {
        numerator += double(f * (f - 1));
    }
    return numerator / (N * (N - 1));
}

double phi(const std::string& text, int shift) {
    std::array<int, 26> freq = {};
    for (char c : text) freq[std::tolower(c) - 'a']++;
    std::array<double, 26> english_freq = {
            0.08167,  // a
            0.01492,  // b
            0.02782,  // c
            0.04253,  // d
            0.12702,  // e
            0.02228,  // f
            0.02015,  // g
            0.06094,  // h
            0.06966,  // i
            0.00153,  // j
            0.00772,  // k
            0.04025,  // l
            0.02406,  // m
            0.06749,  // n
            0.07507,  // o
            0.01929,  // p
            0.00095,  // q
            0.05987,  // r
            0.06327,  // s
            0.09056,  // t
            0.02758,  // u
            0.00978,  // v
            0.02360,  // w
            0.00150,  // x
            0.01974,  // y
            0.00074   // z
    };
   double total = 0;
    for (int k = 0; k < 26; k++) {
        int actualShift = 0;
        if ((k-shift) > 0) actualShift = k-shift;
        else actualShift = shift-k;
        total += (double(freq[k])/double(text.size())) * english_freq[actualShift];}
    return total;
}

int max(std::vector<double> vector){
    double max = 0;
    double max_idx = 0;
    for (size_t i = 0; i < vector.size(); i++){
        if (vector[i] > max){
            max = vector[i];
            max_idx = i;
        }
    }
    return max_idx;
}

void repetitions(std::string input_file){
    std::vector<std::string> vigneres = readStringsFromFile(input_file);
    std::string vigenere = vigneres[0];
    std::erase(vigenere,' ');
    std::cout << "Size " << vigenere.size() << std::endl;

    std::vector<std::string> splits;
    for (size_t i = 0; i < vigenere.size(); i ++) {
        splits.push_back(vigenere.substr(i, 2));
    }

    sort(splits.begin(),splits.end());

    std::map<std::string, int> dict = {};
    for (size_t i = 0; i < splits.size()-1; i++){
        int match = 0;
        //std::cout << splits[i] << std::endl;
        while (splits[i] == splits[i+1]){
            if (match == 0) dict[splits[i]] = 1;
            dict[splits[i]] += 1;
            i++;
            //std::cout << splits[i] << std::endl;
        }
    }

    //for (auto it : dict)
    //    cout << it.first << ": " << it.second << endl;

    auto Iterator = dict.begin();
    std::vector<std::string> patterns;
    std::vector<std::pair<int,int>> indices;
    for (size_t i = 0; i < dict.size(); i++){
        int idx_1 = -1;
        int idx_2 = -1;
        for (size_t j = 0; j < vigenere.size()-1; j++){
            if (vigenere.substr(j, 2) == Iterator->first){
                if (idx_1 == -1) {
                    idx_1 = j+2;
                    indices.push_back(std::pair(idx_1,-1));
                }
                else {
                    idx_2 = j+2;
                    indices[i].second = idx_2;
                }
            }
        }
        std::string match = Iterator->first;
        while (vigenere[idx_1] == vigenere[idx_2] && idx_2 < vigenere.size()) {
            match += (vigenere[idx_1]);
            idx_1++;
            idx_2++;
        }
        patterns.push_back(match);
        Iterator.operator++();
    }

    for (size_t i = 0; i < patterns.size(); i++){
        for (size_t j = 0; j < patterns.size(); j++){
            bool found = false;
            if (patterns[j].size() > patterns[i].size()){
                size_t subset = patterns[j].find(patterns[i]);
                if (subset != string::npos){
                    std::cout << "Erasing " << patterns[i] << std::endl;
                    patterns.erase(patterns.begin()+i);
                    indices.erase(indices.begin()+i);
                    found = true;
                }
            }
            if (found) break;
        }
    }

    std::vector<int> differences;
    for (int i = 0; i < patterns.size(); i++) {
        int diff = indices[i].second - indices[i].first;
        differences.push_back(diff);
        cout << patterns[i] << " start " << indices[i].first << ", " << indices[i].second <<
             " difference " << diff << endl;
    }

    sort(differences.begin(),differences.end());
    for (auto diff: differences) {
        std::cout << diff << " factors:";
        printDivisors(diff);
        std::cout << std::endl;
    }

    std::cout << indexOfCoincidence(vigenere) << std::endl;
    int num_partitions = 5;

    std::vector<std::string> partitions(num_partitions,"");
    for (size_t i = 0; i < vigenere.size(); i ++) {
        partitions[i%num_partitions] += vigenere[i];
    }
    //for (auto partition : partitions) for (auto part: partition) std::cout << part;

    std::vector<std::vector<double>> phi_values(num_partitions);
    std::vector<int> max_values;
    for (int j = 0; j < num_partitions; j++) {
        for (int i = 0; i < 26; i++) {
            phi_values[j].push_back(phi(partitions[j], i));
        }
        std::cout << std::endl;
        for (auto phi : phi_values[j]) std::cout << " " << phi;
        std::cout << std::endl;
        max_values.push_back(max(phi_values[j]));
    }

    for (auto max : max_values) std::cout << "Keys are " << max << std::endl;
    max_values = {0,12,0,25,4};

    std::string decoded = "";
    for (size_t i = 0; i < partitions[0].size(); i++){
        for (int j = 0; j < num_partitions; j++) {
            int shift = partitions[j][i] - max_values[j];
            if ((partitions[j][i] - 'A' - max_values[j]) < 0)
                shift = 26 + partitions[j][i] - max_values[j];
            decoded += char(shift);
        }
    }
    std::cout << decoded << std::endl;
}

void printPhiValues(std::string ciphertext){
    std::vector<double> phi_values;
    int max_value;
    for (int i = 0; i < 26; i++) {
        phi_values.push_back(phi(ciphertext, i));
    }
    std::cout << std::endl;
    for (auto phi : phi_values) std::cout << " " << phi;
    std::cout << std::endl;
    max_value = max(phi_values);

    std::string decoded = "";
    for (int j = 0; j < ciphertext.size(); j++) {
        int shift = ciphertext[j] - max_value;
        if ((ciphertext[j] - 'A' - max_value) < 0)
            shift = 26 + ciphertext[j] - max_value;
        decoded += char(shift);
    }
    std::cout << decoded << std::endl;
}

std::bitset<64> binaryFromBinaryString(const std::string& binStr) {
    std::bitset<64> bin;
    for (int i = 0; i < 64; i++)
        bin[63 - i] = (binStr[i] == '1');
    return bin;
}

template<size_t OUT, size_t IN, size_t N>
std::bitset<OUT> permute(const std::bitset<IN>& input, const std::array<int, N>& table) {
    std::bitset<OUT> output;
    for (size_t i = 0; i < OUT; i++)
        output[OUT - 1 - i] = input[IN - table[i]];
    return output;
}


std::bitset<32> fFunction(const std::bitset<32>& R, const std::bitset<48>& K, int round) {

    std::bitset<48> expand = permute<48, 32>(R, E);
    std::bitset<48> xored = expand ^ K;
    std::bitset<32> sboxOutput;
    for (int box = 0; box < 8; box++) {
        std::bitset<6> block;
        for (int i = 0; i < 6; i++)
            block[5 - i] = xored[47 - (box * 6 + i)];

        int row = int(block[5])*2 + block[0];
        int col = (int(block[4])*8) + (int(block[3])*4) + (int(block[2])*2) + int(block[1]);
        int val = SBOXES[box][row][col];

        for (int i = 0; i < 4; i++)
            sboxOutput[31 - (box * 4 + i)] = (val >> (3 - i)) & 1;
    }

    std::bitset<32> result = permute<32, 32>(sboxOutput, P);

    return result;
}

std::vector<std::bitset<48>> generateDESSubkeys(const std::bitset<64>& key) {
    std::bitset<56> Kplus = permute<56, 64>(key, PC1);
    std::bitset<28> C, D;
    for (int i = 0; i < 28; i++) {
        C[i] = Kplus[i + 28];
        D[i] = Kplus[i];
    }
    std::vector<std::bitset<48>> subkeys(16);
    for (int round = 0; round < 16; round++) {
        C = (C << SHIFT_SCHEDULE[round]) | (C >> (28 - SHIFT_SCHEDULE[round]));
        D = (D << SHIFT_SCHEDULE[round]) | (D >> (28 - SHIFT_SCHEDULE[round]));

        std::bitset<56> CD;
        for (int i = 0; i < 28; i++) {
            CD[i + 28] = C[i];
            CD[i]      = D[i];
        }
        subkeys[round] = permute<48, 56>(CD, PC2);
    }
    return subkeys;
}

void DESDecrypt(std::string keyStr, std::string ciphertextStr){
    std::bitset<64> ciphertext = binaryFromBinaryString(ciphertextStr);

    std::bitset<64> key = binaryFromBinaryString(keyStr);
    std::vector<std::bitset<48>> subkeys = generateDESSubkeys(key);

    std::cout << "Subkeys:\n";
    for (int i = 0; i < 16; i++) {
        std::cout << "K " << (i + 1) << " = ";
        for (int b = 47; b >= 0; --b)
            std::cout << subkeys[i][b];
        std::cout << "\n";
    }
    std::bitset<32> L, R;
    std::bitset<64> ip = permute<64, 64>(ciphertext, IP);
    for (int i = 0; i < 32; i++) {
        L[i] = ip[i + 32];
        R[i] = ip[i];
    }

    for (int round = 1; round <= 16; round++) {
        int subkeyIndex = 16 - round;
        std::bitset<32> newL = R;
        std::bitset<32> fOut = fFunction(R, subkeys[subkeyIndex], round);
        std::bitset<32> newR = L ^ fOut;

        std::cout << "F: ";
        for (int i =31; i >= 0; i--) {
            std::cout << fOut[i];
        }
        std::cout << std::endl;
        L = newL;
        R = newR;
    }
    std::bitset<64> combined;
    for (int i = 0; i < 32; i++) {
        combined[i + 32] = R[i];
        combined[i]      = L[i];
    }
    std::bitset<64> final = permute<64, 64>(combined, IP_INV);

    std::cout << "Result: ";
    for (int i =63; i >= 0; i--) {
        std::cout << final[i];
    }
}




int main(int argc, char *argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> " << std::endl;
        return 1;
    }
    std::string input_file = argv[1];
    std::cout << "Input file: " << input_file << std::endl;

    std::string keyStr = "0100110001001111010101100100010101000011010100110100111001000100";
    std::string ciphertextStr = "1100101011101101101000100110010101011111101101110011100001110011";

    DESDecrypt(keyStr,ciphertextStr);

    return 0;
}

// TIP See CLion help at <a
// href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>.
//  Also, you can try interactive lessons for CLion by selecting
//  'Help | Learn IDE Features' from the main menu.