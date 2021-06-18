#include <iostream>					//cout
#include <iomanip>					//setfill setw
#include <string.h>
#include <fstream>
#include <bitset>

using namespace std;

bool DISPLAY = true;	//flaga czy maja byc wyswietlana dane na ekranie; w przypadku false idzie znacznie szybciej
int CONTROL = 2;				//0 - tylko wyswietla, 1 - assembler PID 136, 2 - assembler PID 174				//ustawienie wyswietlania wszystkiego tak wlasciwie
//sprawdza nastepny w kolejnosci pakiet; jezeli S == 1 to zwraca true, w innym przypaddku false
bool checkNextPacket(FILE* pFile, int PID){
	int nextIndex = ftell(pFile)/188;
	int tempPID = 0;
	uint8_t tempBuffer[188];
	int i = 0;
	while (tempPID != PID){
		if(fread(tempBuffer, sizeof(tempBuffer), 1, pFile) == 1){
			tempPID = (int)(tempBuffer[1] & 0x1F)*256 + (int)tempBuffer[2];
			i++;
		} else return false;
	}
	if (tempPID == PID){
		int S = (int)(tempBuffer[1] & 0x40);
		fseek (pFile , -188 * i , SEEK_CUR);
		if (S == 64){
			return true;
		} else return false;
	}
}

//funkcja wyswietla naglowek AF w przypadku AF == 2 albo AF == 3
void displayAF(uint8_t buffer[], int AF){
	cout << "\tAF: ";
		int AFL = (int)buffer[4];
		int PR = (int)((buffer[5] >> 4) & 0x01);
		int OR = (int)((buffer[5] >> 3) & 0x01);
		cout << "L=" << setfill(' ') << setw(3) << (int)buffer[4] << " ";
		cout << "DC=" << (int)((buffer[5] >> 7) & 0x01) << " ";
		cout << "RA=" << (int)((buffer[5] >> 6) & 0x01) << " ";
		cout << "SP=" << (int)((buffer[5] >> 5) & 0x01) << " ";
		cout << "PR=" << (int)((buffer[5] >> 4) & 0x01) << " ";
		cout << "OR=" << (int)((buffer[5] >> 3) & 0x01) << " ";
		cout << "SP=" << (int)((buffer[5] >> 2) & 0x01) << " ";
		cout << "TP=" << (int)((buffer[5] >> 1) & 0x01) << " ";
		cout << "EX=" << (int)((buffer[5] >> 0) & 0x01) << " ";
		if (PR == 1){
			long PCR_base;
			int PCR_ext;
			string PCRF = "";
			
			for (int i = 0; i < 6; i++){
				for (int j = 7; j >= 0; j--){
					if ((((buffer[6+i] >> j) & 0x01)))
					PCRF += "1";
					else PCRF += "0";
				}
			}
			bitset<48> foo (PCRF);
			PCR_base = (foo>>15).to_ullong();
			PCR_ext = (foo &= 0x1FF).to_ullong();
			int PCR = PCR_base * 300 + PCR_ext;
			cout << "PCR=" << PCR << " ";
			cout << "Time=" << (float)PCR/27000000 << " ";
		}
		if (OR == 1){
			long OPCR_base;
			int OPCR_ext;
			string OPCRF = "";
			
			for (int i = 0; i < 6; i++){
				for (int j = 7; j >= 0; j--){
					if ((((buffer[6+i] >> j) & 0x01)))
					OPCRF += "1";
					else OPCRF += "0";
				}
			}
			bitset<48> foo (OPCRF);
			OPCR_base = (foo>>15).to_ullong();
			OPCR_ext = (foo &= 0x1FF).to_ullong();
			int OPCR = OPCR_base * 300 + OPCR_ext;
			cout << "OPCR=" << OPCR << " ";
			cout << "Time=" << (float)OPCR/27000000 << " ";
		} else if (OR == 1 and PR == 1){
			long PCR_base;
			int PCR_ext;
			string PCRF = "";
			
			for (int i = 0; i < 6; i++){
				for (int j = 7; j >= 0; j--){
					if ((((buffer[6+i] >> j) & 0x01)))
					PCRF += "1";
					else PCRF += "0";
				}
			}
			bitset<48> foo (PCRF);
			PCR_base = (foo>>15).to_ullong();
			PCR_ext = (foo &= 0x1FF).to_ullong();
			int PCR = PCR_base * 300 + PCR_ext;
			cout << "PCR=" << PCR << " ";
			cout << "Time=" << (float)PCR/27000000 << " ";
			
			long OPCR_base;
			int OPCR_ext;
			string OPCRF = "";
			
			for (int i = 0; i < 6; i++){
				for (int j = 7; j >= 0; j--){
					if ((((buffer[12+i] >> j) & 0x01)))
					OPCRF += "1";
					else OPCRF += "0";
				}
			}
			bitset<48> boo (OPCRF);
			OPCR_base = (boo>>15).to_ullong();
			OPCR_ext = (boo &= 0x1FF).to_ullong();
			int OPCR = OPCR_base * 300 + OPCR_ext;
			cout << "OPCR=" << OPCR << " ";
			cout << "Time=" << (float)OPCR/27000000 << " ";
		}
		if (AF == 2) cout << "Stuffing=" << 183 << " ";
		else if (AFL > 1 and PR == 0 and OR == 0) cout << "Stuffing=" << AFL-1 << " ";
		else if (AFL > 1 and (PR == 1 or OR == 1))cout << "Stuffing=" << AFL-1-6 << " ";
}

//wyswietla zawartosc pol PTS i DTS w przypadku wystapienia flag
void displayPTS_DTS(uint8_t buffer[], int offset, int flag){
	string PTS = "";
	string DTS = "";
	
	if (flag == 2){
		for (int i = 0; i < 5; i++){
			for (int j = 7; j >= 0; j--){
				if ((((buffer[i + offset + 2] >> j) & 0x01)))
					PTS += "1";
					else PTS += "0";
			}
		}
		
		bitset<40> bPTS (PTS);
		bPTS = bPTS &= 0xEFFFEFFFE;
		PTS = bPTS.to_string<char,std::string::traits_type,std::string::allocator_type>();
		PTS = PTS.substr(4,3) + PTS.substr(8,15) + PTS.substr(24,15);
		bitset<33> bPTSpost(PTS);
		cout << "PTS=" << bPTSpost.to_ullong() << " ";
		cout << "Time=" << (float)(bPTSpost.to_ullong())/90000 << " ";
	} else if (flag == 3){
		for (int i = 0; i < 5; i++){
			for (int j = 7; j >= 0; j--){
				if ((((buffer[i + offset + 2] >> j) & 0x01)))
				PTS += "1";
				else PTS += "0";
			}
		}
	
		bitset<40> bPTS (PTS);
		bPTS = bPTS &= 0xEFFFEFFFE;
		PTS = bPTS.to_string<char,std::string::traits_type,std::string::allocator_type>();
		PTS = PTS.substr(4,3) + PTS.substr(8,15) + PTS.substr(24,15);
		bitset<33> bPTSpost(PTS);
		cout << "PTS=" << bPTSpost.to_ullong() << " ";
		cout << "Time=" << (float)(bPTSpost.to_ullong())/90000 << " ";
		
		for (int i = 0; i < 5; i++){
			for (int j = 7; j >= 0; j--){
				if ((((buffer[i + offset + 7] >> j) & 0x01)))
				DTS += "1";
				else DTS += "0";
			}
		}
	
		bitset<40> bDTS (DTS);
		bDTS = bDTS &= 0xEFFFEFFFE;
		DTS = bDTS.to_string<char,std::string::traits_type,std::string::allocator_type>();
		DTS = DTS.substr(4,3) + DTS.substr(8,15) + DTS.substr(24,15);
		bitset<33> bDTSpost(DTS);
		cout << "DTS=" << bDTSpost.to_ullong() << " ";
		cout << "Time=" << (float)(bDTSpost.to_ullong())/90000 << " ";
	}
}

class PES_Header{
	public:
		uint32_t sizeOfPacket;
		int PTSFlag;
		PES_Header(){
			sizeOfPacket = 0;
			PTSFlag = 0;
		};
		~PES_Header(){};
		uint32_t getSize(){return sizeOfPacket;}
		void setPTSFlag(int i){PTSFlag = i;}
		int getPTSFlag(){return PTSFlag;}
		void resetSize(){sizeOfPacket = 0;}
		void displayPES(uint8_t buffer[], int PID, FILE* pFile);
		void appendHeader(int size){sizeOfPacket += size;};
};

//Wyswietla naglowek PES w przypadku PID=136 albo PID=174
void PES_Header::displayPES(uint8_t buffer[], int PID, FILE* pFile){
	int AF = (int)(buffer[3] & 0x30)/16;
	int AFL = (int)buffer[4];
	int S = (int)((buffer[1] >> 6) & 0x01);
	int CC = (int)(buffer[3] & 0x0F);
	
	if (AF == 1){
		AFL = 0;
	}
	int iPTSoffset = AFL + 12;
	int iPESoffset = AFL + 5;
	long Len;
	bool suddenDisplayEnd = checkNextPacket(pFile, PID);
	
	string PES = "";
	for (int i = 0; i < 6; i++){
		for (int j = 7; j >= 0; j--){
			if ((((buffer[i + iPESoffset] >> j) & 0x01)))
			PES += "1";
			else PES += "0";
		}
	}
	
	bitset<48> foo (PES);
	long PSCP = ((foo>>24) &= 0xFFFFFF).to_ullong();
	bitset<48> roo (PES);
	long SID = ((roo>>16) &= 0xFF).to_ullong();
	bitset<48> boo (PES);
	long L = (boo &= 0xFFFF).to_ullong();
	int mL = ((uint16_t)buffer[AFL+5+4] << 8) | (uint16_t)buffer[AFL+5+5];
	
	string PTSflag = "";
	
	if (S == 1){
		for (int i = 0; i < 1; i++){
			for (int j = 7; j >= 0; j--){
				if ((((buffer[i + iPTSoffset] >> j) & 0x01)))
				PTSflag += "1";
				else PTSflag += "0";
			}
		}

		bitset<8> bPTSflag (PTSflag);
		bPTSflag = (bPTSflag &= 0xC0) >>= 6;
		if (bPTSflag == 0x2) setPTSFlag(2);
		else if (bPTSflag == 0x3) setPTSFlag(3);
		
		if (AF == 2 or AF == 3){
			if (DISPLAY) cout << "size: " << 184 - (int)buffer[4] - 1  << " ";
				appendHeader(184 - (int)buffer[4] - 1);
		}

		cout << "\tStarted PES:\t";
		cout << "PESC=" << PSCP << " SID=" << SID << " L=" << mL << " ";
		if (bPTSflag == 0x2) displayPTS_DTS(buffer, iPTSoffset, 2);
		else if (bPTSflag == 0x3) displayPTS_DTS(buffer, iPTSoffset, 3);
	} else if (suddenDisplayEnd == true){
		cout << "\tFinished PES:\t";
		appendHeader(184 - (int)buffer[4] - 1);
		if (getPTSFlag() == 2){
			cout << "PcktLen=" << getSize() << " HeadLen=" << 14 << " DataLen=" << getSize()-14 << " ";
		} else if (getPTSFlag() == 3){
			cout << "PcktLen=" << getSize() << " HeadLen=" << 19 << " DataLen=" << getSize()-19 << " ";
		}
		cout << " " << getSize() << " ";
		resetSize();
	} else {
		appendHeader(184);
		cout << "\tContinue PES\t";
	} 
	cout << " " << getSize() << " ";
}

//funkcja wyswietla podstawowe wartosci z 4 pierwszych bajtow
void display(uint8_t buffer[], int TS_PacketId, FILE* pFile){
	int AF = (int)(buffer[3] & 0x30)/16;
	int S = (int)((buffer[1] >> 6) & 0x01);
	int PID = (int)(buffer[1] & 0x1F)*256 + (int)buffer[2];
	int AFL = (int)buffer[4];
	cout << setfill('0') << setw(10) << (TS_PacketId) << " TS: ";
	cout << "SB=" << (int)buffer[0] << " ";
	cout << "E=" << (int)((buffer[1] >> 7) & 0x01) << " ";
	cout << "S=" << (int)((buffer[1] >> 6) & 0x01) << " ";
	cout << "P=" << (int)((buffer[1] >> 5) & 0x01) << " ";
	cout << "PID=" << setfill(' ') << setw(4) << (int)(buffer[1] & 0x1F)*256 + (int)buffer[2] << " ";
	cout << "TSC=" << (int)(buffer[3] & 0xC0)/64 << " ";
	cout << "AF=" << (int)(buffer[3] & 0x30)/16 << " ";
	cout << "CC=" << setfill(' ') << setw(2) << (int)(buffer[3] & 0x0F) << " ";
	if (AF == 2 or AF == 3){
		displayAF(buffer, AF);
	}
}

//klasa odpowiedzialna za assembling pakietow PES
class PES_Assembler{
    protected:
        uint8_t* Buffer;
        uint64_t currentSize;
        uint64_t bufferSize;
        uint64_t offset;
        bool start;
        bool seekSS;
        bool suddenEnd;
    public:
        PES_Assembler(){
        	offset = 0;
        	currentSize = 0;
            bufferSize = 16384000;
            Buffer = new uint8_t[bufferSize];
            suddenEnd = false;
            seekSS = false;
            start = false;
        }
        ~PES_Assembler() {};

        void TakePacket(uint8_t buffer[], FILE* pFile);
        uint8_t* getPacket() {return Buffer;}
        
        void resetBuffer();
        void appendBuffer(const uint8_t* Data, int32_t Size);
        
        int32_t getNumPacketBytes(){return currentSize;}
};

//funkcja resetujaca bufor
void PES_Assembler::resetBuffer(){
	delete [] Buffer;
	currentSize = 0;
	Buffer = new uint8_t[bufferSize];
	start = false;
}

//funkcja dodajaca dany pakiet do bufora
void PES_Assembler::appendBuffer(const uint8_t* Data, int32_t Size){
	if (DISPLAY) cout << " bufferSize: " << currentSize << " " << "Size: " << Size << " ";
	copy(Data+(188-Size), Data+188, Buffer+(currentSize));
    currentSize += Size;
    if (DISPLAY) cout << "bufferSize after: " << currentSize << "\t";
}

//funkcja pobiera pakiet, odczytuje go i podaje do bufora
void PES_Assembler::TakePacket(uint8_t buffer[], FILE* pFile){
	int CC = (int)(buffer[3] & 0x0F);
	int AF = (int)(buffer[3] & 0x30)/16;
	int S = (int)((buffer[1] >> 6) & 0x01);
	int AFL;
	int PID = (int)(buffer[1] & 0x1F)*256 + (int)buffer[2];
	uint32_t size;

	if (AF == 2 or AF == 3){
		if (DISPLAY) cout << "size: " << 184 - (int)buffer[4] - 1  << " ";
		size = 184 - (int)buffer[4] - 1;
		AFL = (int)buffer[4] + 1;
	} else {
		size = 184;
		AFL = 0;
	}

	if (S == 1){
		suddenEnd = false;
		
		offset = ((uint16_t)buffer[AFL+4+4] << 8) | (uint16_t)buffer[AFL+4+5] + 6;
		if (DISPLAY) cout << "offset:" <<  offset << " ";
		if (offset-6 == 0) seekSS = true;
		if (DISPLAY) cout << " Assembly started" << " ";
	}
	
	if(seekSS == true){
		if (checkNextPacket(pFile, PID)){
			suddenEnd = true;
			seekSS = false;
		}
	}
	
	appendBuffer(buffer, size);
	
	if (currentSize == offset and PID == 136){
		
		if (DISPLAY) cout << " Assembly finished136" << " ";
		if (DISPLAY) cout << "PES: Len=" << this->getNumPacketBytes() << " ";
		
		ofstream output("PID136.mp3", ios::app | ios::binary);
		output.write(reinterpret_cast<char*>(this->getPacket()),this->getNumPacketBytes());
		output.close();
		resetBuffer();
	} else if ((currentSize == offset and PID == 174 and seekSS == false) or suddenEnd == true){
		
		if (DISPLAY) cout << " Assembly finished174" << " ";
		if (DISPLAY) cout << "PES: Len=" << this->getNumPacketBytes() << " ";
		
		ofstream output1("PID174.264", ios::app | ios::binary);
		output1.write(reinterpret_cast<char*>(this->getPacket()),this->getNumPacketBytes());
		output1.close();
		resetBuffer();
	} else if (DISPLAY and S != 1) cout << " Assembly continue" << " ";
}

int main( int argc, char *argv[ ], char *envp[ ]){
	//duch maszyny poblogoslawil ten program i dziala
	//variables
	FILE* pFile;
	long lSize;									//Liczba wczytanych bitów bajtów
	
	PES_Assembler PES_Assembler136;
	PES_Assembler PES_Assembler174;
	PES_Header PES_Header;
	
	uint8_t buffer[188];
	int32_t TS_PacketId = 0;
	int32_t AF = 1;						//2 - tylko AF, 3 - AF i payload
	int32_t PID = 0;					//PID w celu segregacji
	
	//--------------------------------control variables------------------------------------------------
	
	//-------------------------------------------------------------------------------------------------
	
	//opening of file
	pFile = fopen("example_new.ts", "rb");
	if (pFile == NULL) {
		return 3;
	}
	
	//getting file size
	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);
	
	if (DISPLAY){
		cout << "Size of loaded file: " << lSize << "\n";
		system("pause");
	} 
	
	//reseting the output files
	if (CONTROL == 1){
		ofstream output1("PID136.mp3");
		output1.close();
	} else if (CONTROL == 2){
		ofstream output2("PID174.264");
		output2.close();
	}
	
 	
 	//main loop
	while(!feof(pFile)){
	//while(TS_PacketId < 50000){
		fread(buffer, sizeof(buffer), 1, pFile);
		
		PID = (int)(buffer[1] & 0x1F)*256 + (int)buffer[2];
		
		if (CONTROL == 0){
			if (DISPLAY) display(buffer, TS_PacketId, pFile);
			if (DISPLAY and (PID == 136 or PID == 174)) PES_Header.displayPES(buffer, PID, pFile);
			if (DISPLAY) cout << "\n";
		}
		if (PID == 136 and CONTROL == 1){
			if (DISPLAY) display(buffer, TS_PacketId, pFile);
			if (DISPLAY) PES_Header.displayPES(buffer, PID, pFile);
			PES_Assembler136.TakePacket(buffer, pFile);
		} 
		if (PID == 174 and CONTROL == 2){
			if (DISPLAY) display(buffer, TS_PacketId, pFile);
			if (DISPLAY) PES_Header.displayPES(buffer, PID, pFile);
			PES_Assembler174.TakePacket(buffer, pFile);
			if (DISPLAY) cout << "\n";
		}
		TS_PacketId++;
	}
	fclose(pFile);
	return 0;
}
