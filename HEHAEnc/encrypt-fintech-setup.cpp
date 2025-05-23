//HOMOMORPHIC EVALUATION OF BINARY DECISION TREE FROM OPENFHE : CLIENT SIDE

#include "openfhe.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <chrono>

// header files needed for serialization
#include "ciphertext-ser.h"
#include "cryptocontext-ser.h"
#include "key/key-ser.h"
#include "scheme/bgvrns/bgvrns-ser.h"

using namespace lbcrypto;
using namespace std::chrono;

const std::string DATAFOLDER = "data";
const std::string RESULTSFOLDER = "results";
const std::string PrivateKeyFolder = "private_data";
const std::string TimingFolder = "timing";

//binary decision trees
typedef struct bdt
{
	std::vector<int64_t> root;
	bdt* left = nullptr;
	bdt* right = nullptr;
} bdt;

//splitting a vector in two (in order to build a tree)
std::vector<std::vector<std::vector<int64_t>>> split(std::vector<std::vector<int64_t>> tags, int depth)
{
	std::vector<std::vector<std::vector<int64_t>>> result;
	result.push_back({});
	result.push_back({});

	int cpt = 1;

	
	for(int i=1; i<depth; i++)
	{
		for(int j=0; j<pow(2, i-1); j++)
		{
			(result[0]).push_back(tags[cpt]);
			cpt++;
		}
		for(int j=0; j<pow(2, i-1); j++)
		{
			(result[1]).push_back(tags[cpt]);
			cpt++;
		}
	}
	
	return result;
}

// reading data from a file (new)
std::vector<std::vector<int64_t>> readTreeData(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("could not open the file " + filePath);
    }

    std::vector<std::vector<int64_t>> treeData;
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::vector<int64_t> levelData;
        int64_t value;

        while (iss >> value) {
            levelData.push_back(value);
        }
        treeData.push_back(levelData);
    }

    file.close();
    return treeData;
}

//Tree building (new)
void buildTree(bdt& node, const std::vector<std::vector<int64_t>>& treeData, std::size_t level, std::size_t index) {
    if (level >= treeData.size() || index >= treeData[level].size()) {
        return; // No data for this node
    }

    // Filling the current node
    node.root.push_back(treeData[level][index]);

    // Building the left and right child nodes if the next level does exist
    if (level + 1 < treeData.size()) {
        node.left = new bdt();
        node.right = new bdt();
        buildTree(*node.left, treeData, level + 1, 2 * index);     // left child
        buildTree(*node.right, treeData, level + 1, 2 * index + 1); // right child
    }
}

//building a tree from a vector
bdt build_tree(std::vector<std::vector<int64_t>> tags, int depth)
{
	bdt tree;
	tree.root = tags[0];
	std::cout << tree.root << std::endl;
	tree.left = new bdt();
	tree.right = new bdt();
	if(depth>1)
	{
	   *(tree.left) = build_tree(split(tags, depth)[0], depth-1);
	   *(tree.right) = build_tree(split(tags, depth)[1], depth-1);
	}
	else
	{
		tree.left = NULL;
		tree.right = NULL;
	}
	return tree;
}

//binary decision trees encoded as plaintexts
typedef struct bdt_pt
{
	Plaintext root;
	bdt_pt* left;
	bdt_pt* right;
} bdt_pt;

//encrypted binary decision trees
typedef struct bdt_ct
{
	Ciphertext<DCRTPoly> root;
	bdt_ct* left;
	bdt_ct* right;
} bdt_ct;

//encoding a binary decision tree
bdt_pt bdt_encode(CryptoContext<DCRTPoly> cc, bdt tree)
{
	bdt_pt result;
	result.root = cc->MakePackedPlaintext(tree.root);
	result.left = new bdt_pt();
	result.right = new bdt_pt();
	if(tree.left!=NULL)
	{
	  *(result.left) = bdt_encode(cc, *(tree.left));
	}
	else
	{
	  result.left = NULL;
	}
	if(tree.right!=NULL)
	{
	  *(result.right) = bdt_encode(cc, *(tree.right));
	}
	else
	{
	  result.right=NULL ;
	}
	return result;
}

//encryption of a binary decision tree
bdt_ct bdt_encrypt(CryptoContext<DCRTPoly> cc, bdt_pt tree, const PublicKey<DCRTPoly> pk)
{
	bdt_ct result;
	result.root = cc->Encrypt(pk, tree.root);
	result.left = new bdt_ct();
	result.right = new bdt_ct();
	if(tree.left!=NULL)
	{
	  *(result.left) = bdt_encrypt(cc, *(tree.left), pk);
	}
	else
	{
	  result.left = NULL;
	}
	if(tree.right!=NULL)
	{
	  *(result.right) = bdt_encrypt(cc, *(tree.right), pk);
	}
	else
	{
	  result.right = NULL;
	}
	return result;
}

// subfunction for recursive serialization of an encrypted bdt (from depth-first search)
int ebdt_serialize_switched(bdt_ct tree, std::string name, int i)
{
	if (!Serial::SerializeToFile(RESULTSFOLDER + "/" + name + std::to_string(i) + ".txt", tree.root, SerType::BINARY)) {
        std::cerr << "Error writing serialization of node " << i << "to ciphertext" << i << ".txt" << std::endl;
    }
    std::cout << "serialized ciphertext " << i << std::endl;
    i++;
    
    if(tree.left !=NULL)
    {
	   i = ebdt_serialize_switched(*(tree.left), name, i);	
    }
    
    if(tree.right !=NULL)
    {
		i = ebdt_serialize_switched(*(tree.right), name, i);	
    }
    
    return i;
}

// serialization of an encrypted bdt from depth-first search
void ebdt_serialize(bdt_ct tree, std::string name)
{
	ebdt_serialize_switched(tree, name, 0);
}

// Clean up memory for bdt structure
void freeTree(bdt* tree) {
    if (tree == nullptr) return;
    
    if (tree->left != nullptr) {
        freeTree(tree->left);
        delete tree->left;
    }
    
    if (tree->right != nullptr) {
        freeTree(tree->right);
        delete tree->right;
    }
}

// Clean up memory for bdt_pt structure
void freePTTree(bdt_pt* tree) {
    if (tree == nullptr) return;
    
    if (tree->left != nullptr) {
        freePTTree(tree->left);
        delete tree->left;
    }
    
    if (tree->right != nullptr) {
        freePTTree(tree->right);
        delete tree->right;
    }
}

// Clean up memory for bdt_ct structure
void freeCTTree(bdt_ct* tree) {
    if (tree == nullptr) return;
    
    if (tree->left != nullptr) {
        freeCTTree(tree->left);
        delete tree->left;
    }
    
    if (tree->right != nullptr) {
        freeCTTree(tree->right);
        delete tree->right;
    }
}

// Function to write timing results to file
void writeTimingToFile(const std::string& filename, const std::string& operation, double timeInMs) {
    std::ofstream outfile(TimingFolder + "/" + filename, std::ios::app);
    if (!outfile) {
        std::cerr << "Could not open timing file for writing" << std::endl;
        return;
    }
    outfile << operation << "," << timeInMs << std::endl;
    outfile.close();
}

/////////////////////////////////////////////
//                                         //
//               |MAIN|                    //
//                                         //
/////////////////////////////////////////////

int main(int argc, char* argv[])
{
    // Default parameters
    uint32_t multDepth = 8;
    uint32_t plainModulus = 65537;
    uint32_t securityLevel = 128; // Default security level
    std::string treeFile = DATAFOLDER + "/tree.txt";
    std::string dataFile = DATAFOLDER + "/data.txt";
    std::string timingFile = "timing_encryption.csv";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--depth" && i + 1 < argc) {
            multDepth = std::stoi(argv[++i]);
        } else if (arg == "--modulus" && i + 1 < argc) {
            plainModulus = std::stoi(argv[++i]);
        } else if (arg == "--security" && i + 1 < argc) {
            securityLevel = std::stoi(argv[++i]);
            if (securityLevel != 128 && securityLevel != 192 && securityLevel != 256) {
                std::cout << "Warning: Security level must be 128, 192, or 256. Setting to default (128)." << std::endl;
                securityLevel = 128;
            }
        } else if (arg == "--tree" && i + 1 < argc) {
            treeFile = DATAFOLDER + "/" + argv[++i];
        } else if (arg == "--data" && i + 1 < argc) {
            dataFile = DATAFOLDER + "/" + argv[++i];
        } else if (arg == "--timing" && i + 1 < argc) {
            timingFile = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                      << "Options:\n"
                      << "  --depth N       Set multiplicative depth (default: 8)\n"
                      << "  --modulus N     Set plaintext modulus (default: 65537)\n"
                      << "  --security N    Set security level (128, 192, or 256) (default: 128)\n"
                      << "  --tree FILE     Specify tree input file (default: tree.txt)\n"
                      << "  --data FILE     Specify data input file (default: data.txt)\n"
                      << "  --timing FILE   Specify timing output file (default: timing_encryption.csv)\n"
                      << "  --help          Display this help message\n";
            return 0;
        }
    }
    
    // Create results directory if it doesn't exist
    //system(("mkdir -p " + RESULTSFOLDER).c_str());
    //system(("mkdir -p " + PrivateKeyFolder).c_str());
    
    // Initialize timing file with header
    std::ofstream timingInit(TimingFolder + "/" + timingFile);
    timingInit << "Operation,TimeInMilliseconds" << std::endl;
    timingInit.close();
    
    auto startTotal = high_resolution_clock::now();
    
    // Cryptocontext setting with configurable parameters
    auto startContext = high_resolution_clock::now();
    CCParams<CryptoContextBGVRNS> parameters;
    parameters.SetMultiplicativeDepth(multDepth);
    parameters.SetPlaintextModulus(plainModulus);
    
    // Set the security level based on user input
    SecurityLevel secLevelEnum;
    if (securityLevel == 128) {
        secLevelEnum = HEStd_128_classic;
    } else if (securityLevel == 192) {
        secLevelEnum = HEStd_192_classic;
    } else if (securityLevel == 256) {
        secLevelEnum = HEStd_256_classic;
    } else {
        std::cout << "Warning: Invalid security level. Defaulting to 128-bit." << std::endl;
        secLevelEnum = HEStd_128_classic;
    }
    parameters.SetSecurityLevel(secLevelEnum);
      
    CryptoContext<DCRTPoly> cc = GenCryptoContext(parameters);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    auto stopContext = high_resolution_clock::now();
    double contextTime = duration_cast<milliseconds>(stopContext - startContext).count();
    writeTimingToFile(timingFile, "CryptoContext_Creation", contextTime);
    
    // Key generation
    auto startKeyGen = high_resolution_clock::now();
    KeyPair<DCRTPoly> keyPair;
    keyPair = cc->KeyGen();
    const PublicKey<DCRTPoly> pk = keyPair.publicKey;
    const PrivateKey<DCRTPoly> sk = keyPair.secretKey;
      
    cc->EvalMultKeyGen(sk);
    auto stopKeyGen = high_resolution_clock::now();
    double keyGenTime = duration_cast<milliseconds>(stopKeyGen - startKeyGen).count();
    writeTimingToFile(timingFile, "Key_Generation", keyGenTime);
    
    // Serialize cryptocontext
    auto startSerialization = high_resolution_clock::now();
    if (!Serial::SerializeToFile(RESULTSFOLDER + "/cryptocontext.txt", cc, SerType::BINARY)) {
        std::cerr << "Error writing serialization of the crypto context to results/cryptocontext.txt" << std::endl;
        return 1;
    }
    // Save cryptocontext.txt also in /app/cryptocontext/
    if (!Serial::SerializeToFile("cryptocontext/cryptocontext.txt", cc, SerType::BINARY)) {
        std::cerr << "Error writing serialization of the crypto context to cryptocontext/cryptocontext.txt" << std::endl;
        return 1;
    }
    
    // Serialize the public key
    if (!Serial::SerializeToFile(RESULTSFOLDER + "/key-public.txt", keyPair.publicKey, SerType::BINARY)) {
        std::cerr << "Error writing serialization of public key to key-public.txt" << std::endl;
        return 1;
    }
    
    // Serialize the secret key
    if (!Serial::SerializeToFile(PrivateKeyFolder + "/key-private.txt", keyPair.secretKey, SerType::BINARY)) {
        std::cerr << "Error writing serialization of private key to key-private.txt" << std::endl;
        return 1;
    }
    
    // Serialize the evaluation key
    std::ofstream emkeyfile(RESULTSFOLDER + "/" + "key-eval-mult.txt", std::ios::out | std::ios::binary);
    if (emkeyfile.is_open()) {
        if (cc->SerializeEvalMultKey(emkeyfile, SerType::BINARY) == false) {
            std::cerr << "Error writing serialization of the eval mult keys to key-eval-mult.txt" << std::endl;
            return 1;
        }
        emkeyfile.close();
    } else {
        std::cerr << "Error serializing eval mult keys" << std::endl;
        return 1;
    }
    auto stopSerialization = high_resolution_clock::now();
    double serializationTime = duration_cast<milliseconds>(stopSerialization - startSerialization).count();
    writeTimingToFile(timingFile, "Keys_Serialization", serializationTime);
    
    //////////////////////////////////////////////////////
    // ENCRYPTING A BINARY DECISION TREE //
    //////////////////////////////////////////////////////
    bdt tree;
    auto startTreeLoad = high_resolution_clock::now();
    try {
        // Reading the data from the tree input file
        std::vector<std::vector<int64_t>> treeData = readTreeData(treeFile);

        // Building the tree directly in a `bdt` object
        buildTree(tree, treeData, 0, 0);
        std::cout << "Decision tree successfully built from the input file." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    auto stopTreeLoad = high_resolution_clock::now();
    double treeLoadTime = duration_cast<milliseconds>(stopTreeLoad - startTreeLoad).count();
    writeTimingToFile(timingFile, "Tree_Loading", treeLoadTime);
      
    std::cout << "Nodes of the binary decision tree (step by step and from left to right)" << std::endl;
    // Print the tree nodes (this might need to be adapted for arbitrary depth trees)
    std::cout << tree.root;
    if (tree.left) std::cout << (tree.left)->root;
    if (tree.right) std::cout << (tree.right)->root;
    if (tree.left && tree.left->left) std::cout << (tree.left)->left->root;
    if (tree.left && tree.left->right) std::cout << (tree.left)->right->root;
    if (tree.right && tree.right->left) std::cout << (tree.right)->left->root;
    if (tree.right && tree.right->right) std::cout << (tree.right)->right->root;
    std::cout << std::endl;
      
    // Encoding
    auto startEncoding = high_resolution_clock::now();
    bdt_pt encoded_tree = bdt_encode(cc, tree);
    auto stopEncoding = high_resolution_clock::now();
    double encodingTime = duration_cast<milliseconds>(stopEncoding - startEncoding).count();
    writeTimingToFile(timingFile, "Tree_Encoding", encodingTime);
      
    // Encrypting
    auto startEncrypting = high_resolution_clock::now();
    bdt_ct encrypted_tree = bdt_encrypt(cc, encoded_tree, pk);
    auto stopEncrypting = high_resolution_clock::now();
    double encryptingTime = duration_cast<milliseconds>(stopEncrypting - startEncrypting).count();
    writeTimingToFile(timingFile, "Tree_Encryption", encryptingTime);
      
    // Serialization
    auto startTreeSer = high_resolution_clock::now();
    ebdt_serialize(encrypted_tree, "encrypted_tree");
    auto stopTreeSer = high_resolution_clock::now();
    double treeSerTime = duration_cast<milliseconds>(stopTreeSer - startTreeSer).count();
    writeTimingToFile(timingFile, "Tree_Serialization", treeSerTime);
      
    //////////////////////////////////////////////////////
    // ENCRYPTING INPUT DATA AS ANOTHER TREE //
    //////////////////////////////////////////////////////
    bdt data;
    auto startDataLoad = high_resolution_clock::now();
    try {
        // Reading the data from the client data file
        std::vector<std::vector<int64_t>> dataData = readTreeData(dataFile);

        // Building the tree directly in a `bdt` object
        buildTree(data, dataData, 0, 0);
        std::cout << "Data tree successfully built from the input file." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    auto stopDataLoad = high_resolution_clock::now();
    double dataLoadTime = duration_cast<milliseconds>(stopDataLoad - startDataLoad).count();
    writeTimingToFile(timingFile, "Data_Loading", dataLoadTime);
    
    std::cout << "Nodes of the binary data tree (step by step and from left to right)" << std::endl;
    // Print the data nodes
    std::cout << data.root;
    if (data.left) std::cout << (data.left)->root;
    if (data.right) std::cout << (data.right)->root;
    if (data.left && data.left->left) std::cout << (data.left)->left->root;
    if (data.left && data.left->right) std::cout << (data.left)->right->root;
    if (data.right && data.right->left) std::cout << (data.right)->left->root;
    if (data.right && data.right->right) std::cout << (data.right)->right->root;
    std::cout << std::endl;
    
    // Encoding
    auto startDataEncoding = high_resolution_clock::now();
    bdt_pt encoded_data = bdt_encode(cc, data);
    auto stopDataEncoding = high_resolution_clock::now();
    double dataEncodingTime = duration_cast<milliseconds>(stopDataEncoding - startDataEncoding).count();
    writeTimingToFile(timingFile, "Data_Encoding", dataEncodingTime);
    
    // Encrypting
    auto startDataEncrypting = high_resolution_clock::now();
    bdt_ct encrypted_data = bdt_encrypt(cc, encoded_data, pk);
    auto stopDataEncrypting = high_resolution_clock::now();
    double dataEncryptingTime = duration_cast<milliseconds>(stopDataEncrypting - startDataEncrypting).count();
    writeTimingToFile(timingFile, "Data_Encryption", dataEncryptingTime);
    
    // Serialization
    auto startDataSer = high_resolution_clock::now();
    ebdt_serialize(encrypted_data, "encrypted_data");
    auto stopDataSer = high_resolution_clock::now();
    double dataSerTime = duration_cast<milliseconds>(stopDataSer - startDataSer).count();
    writeTimingToFile(timingFile, "Data_Serialization", dataSerTime);
    
    auto stopTotal = high_resolution_clock::now();
    double totalTime = duration_cast<milliseconds>(stopTotal - startTotal).count();
    writeTimingToFile(timingFile, "Total_Encryption_Process", totalTime);
    
    // Clean up memory
    freeTree(&tree);
    freeTree(&data);
    freePTTree(&encoded_tree);
    freePTTree(&encoded_data);
    freeCTTree(&encrypted_tree);
    freeCTTree(&encrypted_data);
    
    // Write configuration to file for reference
    std::ofstream configFile(RESULTSFOLDER + "/config.txt");
    configFile << "Multiplicative Depth: " << multDepth << std::endl;
    configFile << "Plaintext Modulus: " << plainModulus << std::endl;
    configFile << "Security Level: " << securityLevel << std::endl;
    configFile << "Tree File: " << treeFile << std::endl;
    configFile << "Data File: " << dataFile << std::endl;
    configFile.close();
    
    return 0;
}