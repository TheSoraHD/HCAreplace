#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

vector<const char*> HCAQuality {
    "Highest",
    "High",
    "Middle",
    "Low",
    "Lowest"
};

vector<unsigned char> simpleread(const char* file_path) {
    int c;
    vector<unsigned char> buffer;
    FILE* simple_read;
    simple_read = fopen(file_path, "rb");
    if (simple_read) {
        while ((c = getc(simple_read)) != EOF) {
            buffer.push_back(c);
        }
        fclose(simple_read);
    }
    return buffer;
}

void wavEncoder(const char* input, const char* output, const char* quality) {
    char temp[512];
    cout << "Encoding... ";
    sprintf(temp, "VGAudioCli.exe -i %s -o %s --hcaquality %s", input, output, quality);
    system((char*)temp);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        cout << "Usage: HCAreplace AWB/UASSET OriginalHCA NewHCA/NewWAV" << endl << endl;
        cout << "       -Original HCA must be present in .awb/.uasset file." << endl;
        cout << "       -Note: In order to obtain all Original HCAs, you can use a tool like VGMToolbox." << endl << endl;
        cout << "       -New HCA must have the same or less size than the Original HCA!" << endl;
        cout << "       -Otherwise, New WAV will be converted to HCA at the best quality available." << endl << endl;
        cout << "Tool by Svenchu." << endl << endl;
        return -1;
    }

    const char* awbFile = argv[1];
    const char* originalhcaFile = argv[2];
    const char* modifiedhcaFile = argv[3];

    //Get HCA original buffer
    vector<unsigned char> oghca_buffer = simpleread(originalhcaFile);
    int ogSize = oghca_buffer.size();
    cout << "Original HCA size: " << ogSize << endl;

    //Check if modifiedhcaFile is an HCA; if not, encode as many times as needed
    vector<unsigned char> modhca_buffer = simpleread(modifiedhcaFile);
    unsigned char hca_header[4] = { 0x48,0x43,0x41,0x00 }; //"HCA_"
    unsigned char header_buffer[4];
    int header = 0;
    while (header < 4) {
        header_buffer[header] = modhca_buffer[header];
        header++;
    }
    if (header_buffer != hca_header) {
        bool done = false;
        int quality = 0;
        while (!done && quality < 5) {
            wavEncoder(modifiedhcaFile, "output.hca", HCAQuality[quality]);
            vector<unsigned char> newBuffer = simpleread("output.hca");
            cout << "Encoded HCA size with quality " << HCAQuality[quality] << ": " << newBuffer.size() << endl;
            if ((ogSize >= newBuffer.size()) && (newBuffer.size() != 0)) {
                done = true;
                modhca_buffer = newBuffer;
            }
            quality++;
        }
        if (!done) {
            cout << "Can't compress HCA enough! Use a smaller WAV and try again." << endl;
            return -1;
        }
    }

    //Check HCAs same size.
    cout << "Modified HCA size: " << modhca_buffer.size() << endl;

    if ((ogSize < modhca_buffer.size()) || (modhca_buffer.size() == 0)) {
        cout << "Modified HCA file not valid! Must be lower in size than the Original HCA." << endl;
        return -1;
    }
    
    //Make the Modified HCA have same size than the original one, so AWB keeps aligned.
    int extraBytes = ogSize - modhca_buffer.size();
    modhca_buffer.reserve(modhca_buffer.size() + extraBytes);
    for (int i = 0; i < extraBytes; i++) {
        modhca_buffer.push_back(0x00);
    }

    //Open the AWB file with binary flag.
    int i = 0;
    int c;
    bool found = false;
    FILE *binaryawb;
    unsigned char c2;
    vector<unsigned char> awb_buffer;
    vector<unsigned char> awb_file;
    binaryawb = fopen(awbFile, "rb");

    if (binaryawb)
    {
        while ((c = getc(binaryawb)) != EOF) {
            c2 = static_cast<unsigned char>(c);
            if (!found && c2 == oghca_buffer[i]) {
                i++;
                awb_buffer.push_back(c2);
                if (i == ogSize) {
                    cout << "Original HCA found! Replacing HCA..." << endl;
                    for (int j = 0; j < ogSize; j++)
                        awb_file.push_back(modhca_buffer[j]);
                    i = 0;
                    found = true;
                    awb_buffer.clear();
                }
            }
            else if (i == 0) {
                awb_file.push_back(c2);
            }
            else {
                for(int j = 0; j < awb_buffer.size(); j++)
                    awb_file.push_back(awb_buffer[j]);
                awb_file.push_back(c2);
                awb_buffer.clear();
                i = 0;
            }
        }
        fclose(binaryawb);
    }

    if (!found) {
        cout << "Original HCA not found inside AWB/UASSET..." << endl;
        return -1;
    }

    FILE* outputawb;
    outputawb = fopen(awbFile, "wb+");

    if (outputawb)
    {
        cout << "Overwriting!" << endl;
        for (int j = 0; j < awb_file.size(); j++) {
            putc(awb_file[j], outputawb);
        }

        fclose(outputawb);
    }

    return 0;

}