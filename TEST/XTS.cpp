#include <iostream>//I/O
#include <fstream>//pliki
//#include <Windows.h>		
#include <chrono>//Mierzony czas				
#include <string.h>//do starszych funkcji stringowych
#include <string>//do funkcji stringowych
#include <iomanip>//wypelniacze	
#include <bitset>//bitwise operacje
#include <cassert>//sprawdzanie
using namespace std;


//=================DEKLARACJE 
unsigned long long int byteAmount(FILE* &myFile);//Funkcja zwracaja rozmiar w bajtach. Potrzebna jako rozmiar bufora
bool packetIterator(FILE* sourceFile, int);//Funkcja chodząca po pakiecikach, sprawdzająca czy osiągnięto koniec konkretnego pakietu
uint8_t userInterface();//Interfejs uzytkownika
uint8_t TS_AdaptationField(uint8_t xTS[], int);//Zawartosc AF, obliczanie i wyswietlanie
uint8_t TS_PTS(uint8_t xTS[], int, int);//Zawartosc PTS, obliczanie i wyswietlanie
uint8_t TS_DTS(uint8_t xTS[], int, int);//Zawartosc DTS, obliczanie i wyswietlanie
uint8_t TS_Header(uint8_t xTS[], FILE* sourceFile, int);//Wyswietlanie nagłówka

class TS_PES;//Wyświetlanie danych z PES
class TS_AssemblerPES;//Składanie w całość PES

//=================GLOBALNE ZMIENNE
bool parsingSelector;//Wybor parsowania 0 - fonia, 1 - wizja
bool parsingMode;//Tryb parsowania0 - no terminal, 1 - with terminal
const uint8_t frameSize = 188;//rozmiar(B): Header+AF+Payload
uint8_t xTS[frameSize]; //Pakiety
const char* filename="example_new.ts";//nazwa pliku
FILE* sourceFile; //plik źródłowy jako obiekt
unsigned long long int byteSize=byteAmount(sourceFile);//rozmiar pliku w bajtach

//=================INTERFEJS
uint8_t userInterface()
{	
	cout<<"--------------------------------------------\n";
	cout<<"Projekt zaliczeniowy z przedmiotu Przesylanie Danych Multimedialnych: MPEG-TS Parser \n";
	cout<<"Autor: Michal Lenczyk, nr albumu: 140130\n";
	cout<<"--------------------------------------------\n\n";
	cout<<"Aktualnie wybrany plik do parsowania to: "<<filename;
	cout<<"\nWybierz parsowanie:\n 0 - parsowanie strumienia fonii do formatu .mp3 (PID=136)\n 1 - parsowanie strumienia wizji do formatu .264 (PID=174)\n";
	cout<<"Your input: ";
	cin>>parsingSelector;
	cout<<"\nWybierz tryb parsowania:\n 0 - bez konsoli (jedynie koncowe pliki wyjsciowe)\n 1 - w oknie konsoli (status i wyswietlanie)\n";
	cout<<"Your input: ";
	cin>>parsingMode;
	cout<<"--------------------------------------------\n\n";
	
	return 0;
}
//=================DEFINICJE
class TS_PES{
	private:
		
	public:
		uint64_t sizePES;
		int flagPTS;
		
		uint32_t getSizePES()
		{
			return sizePES-4;
		}
		
		uint8_t setFlagPTS(int i)
		{
			flagPTS = i;
			return 0;
		}
		
		int getFlagPTS()
		{
			return flagPTS;
		}
		
		uint8_t restart()
		{
			sizePES = 0;
			return 0;
		}
		
		uint8_t TS_HeaderPES(uint8_t xTS[], FILE* sourceFile, int PID);
		
		uint8_t addHeaderPES(int size)
		{
			sizePES += size;
			return 0;
		};
		
		TS_PES()
		{
			sizePES = 0;
			flagPTS = 0;
		};
		~TS_PES(){};
};

class TS_AssemblerPES{
    private:
	public:
        bool PES_packetEnd;
        bool PES_lastPacket;
		uint8_t* packetPES;
        uint64_t packetLength;
        uint64_t off;
        
        uint8_t absorbPacket(uint8_t xTS[], FILE* sourceFile);
        uint8_t* getPacketPES() 
		{
			return packetPES;
		}
        uint8_t resetPacketBuff();
        uint8_t addPacketBuff(const uint8_t* packet, int32_t addNewSize);
        int32_t getNumPacketBytes()
		{
			return packetLength;
		}
        
        TS_AssemblerPES()
		{
        	PES_lastPacket = false;
            PES_packetEnd = false;
        	packetPES = new uint8_t[byteSize];
        	packetLength = 0;
            off = 0;
        }
        ~TS_AssemblerPES() {};
};

uint8_t TS_PES::TS_HeaderPES(uint8_t xTS[], FILE* sourceFile, int PID){
	uint16_t AF = static_cast<int>(xTS[3] & 0x30)/0x10;
	uint16_t AFL = static_cast<int>(xTS[4]);
	uint8_t S = static_cast<int>((xTS[1] >> 0x06) & 0x1);
	uint8_t CC = static_cast<int>(xTS[3] & 0x0F);
	short int HeadLen=14;
	
	if (AF == 1){
		AFL = 0;
	}
	int offPTS = AFL + 12;
	int offPES = AFL + 5;
	bool finishedPESPacket = packetIterator(sourceFile, PID);
	
	string PES_01 = "";
	for (int i = 0; i < 6; i++){
		for (int j = 7; j >= 0; j--){
			if ((((xTS[i + offPES] >> j) & 0x01)))
			PES_01 += "1";
			else PES_01 += "0";
		}
	}
	
	bitset<64> convertStringPSCP(PES_01);
	long PSCP = ((convertStringPSCP>>24) &= 0xFFFFFF).to_ullong();
	bitset<64> convertStringSID(PES_01);
	long SID = ((convertStringSID>>16) &= 0xFF).to_ullong();
	int L = (static_cast<uint16_t>(xTS[AFL+9]) << 0x8) | static_cast<uint16_t>(xTS[AFL+10]);
	
	string PTS_01_flag = "";
	
	if (S == 1){
		for (int i = 0; i < 1; i++){
			for (int j = 7; j >= 0; j--){
				if ((((xTS[i + offPTS] >> j) & 0x01)))
				PTS_01_flag += "1";
				else PTS_01_flag += "0";
			}
		}

		bitset<16> convertedFlagPTS (PTS_01_flag);
		convertedFlagPTS = (convertedFlagPTS &= 0xC0) >>= 6;
		if (convertedFlagPTS == 0x2) setFlagPTS(0x2);
		else if (convertedFlagPTS == 0x3) setFlagPTS(0x3);
		
		if (AF == 2 or AF == 3){
				addHeaderPES((frameSize-4) - static_cast<int>(xTS[4] - 1));
		}

		cout << " PES: ";
		cout << "PSCP=" << PSCP << " SID=" << SID << " L=" << L << " ";
		if (convertedFlagPTS == 0x2)
		{
		 	TS_PTS(xTS, 2, offPTS);
		 	//DTS(xTS, offPTS, 2);
		}
		else if (convertedFlagPTS == 3)
		{
			TS_PTS(xTS, 3, offPTS);
			TS_DTS(xTS, 3, offPTS);	
		}
	} else if (finishedPESPacket == true){
		cout << " PES: ";
		addHeaderPES((frameSize-4) - static_cast<int>(xTS[4] - 1));
		if (getFlagPTS() == 2)
		{
			cout << " PacketLen= " << getSizePES() << " HeadLen=" << HeadLen<< " DataLen= " << getSizePES()-HeadLen;
		} else if (getFlagPTS() == 3)
		{
			cout << " PacketLen= " << getSizePES() << " HeadLen=" << HeadLen+5 << " DataLen= " << getSizePES()-(HeadLen+5);
		}
		
		restart();
	} else 
	{
		addHeaderPES((frameSize-4));
		cout << " Continue ";
	} 
	return 0;
}

uint8_t TS_AssemblerPES::resetPacketBuff(){
	delete [] packetPES;
	packetLength = 0;
	packetPES = new uint8_t[byteSize];
	
	return 0;
}

uint8_t TS_AssemblerPES::addPacketBuff(const uint8_t* packet, int32_t addNewSize){
	copy(packet+(frameSize-addNewSize), packet+frameSize, packetPES+(packetLength));
    packetLength = packetLength+addNewSize;
    return 0;
}

uint8_t TS_AssemblerPES::absorbPacket(uint8_t xTS[], FILE* sourceFile){
	uint16_t AFL;
	uint16_t PID = static_cast<int>(xTS[1] & 0x1F)*0x100 + static_cast<int>(xTS[2]);
	uint16_t CC = static_cast<int>(xTS[3] & 0x0F);
	uint16_t AF = static_cast<int>(xTS[3] & 0x30)/0x10;
	uint16_t S = static_cast<int>((xTS[1] >> 6) & 0x01);
	uint32_t packetSize;

	if (AF == 2 or AF == 3)
	{
		packetSize = (frameSize-4) - static_cast<int>(xTS[4]) - 0x01;
		AFL = static_cast<int>(xTS[4]) + 0x01;
	} else 
	{
		packetSize = (frameSize-4);
		AFL = 0;
	}
	if (S == 1){
		PES_lastPacket = false;
		off = ((uint16_t)xTS[AFL+8] << 8) | (uint16_t)xTS[AFL+9] + 6;
		if (off-6 == 0) PES_packetEnd = true;
	}
	
	if(PES_packetEnd == true){
		if (packetIterator(sourceFile, PID)){
			PES_lastPacket = true;
			PES_packetEnd = false;
		}
	}
	
	addPacketBuff(xTS, packetSize);
	
	if (packetLength == off && PID == 136){
		
		ofstream resultAudio("PID136.mp3", ios::app | ios::binary);
		resultAudio.write(reinterpret_cast<char*>(this->getPacketPES()),this->getNumPacketBytes());//od ktorego do ktorego pakietu
		resetPacketBuff();
	} else if ((packetLength == off && PID == 174 && PES_packetEnd == false) or PES_lastPacket == true){
		
		ofstream resultVideo("PID174.264", ios::app | ios::binary);// format RBSP
		resultVideo.write(reinterpret_cast<char*>(this->getPacketPES()),this->getNumPacketBytes());//od ktorego do ktorego pakietu 
		resetPacketBuff();
	}
	if(parsingMode) cout<<endl;
	return 0;
}

unsigned long long int byteAmount(FILE* &myFile)
{
	fseek (myFile , 0 , SEEK_END);//szukaj końca
	unsigned long int byteSize = ftell (myFile);//zwróć wartość
	rewind (myFile);//powrot na początek pliku by móc przejść do kolejnych dzialan, tj. zaczynamy parsowanie
	return byteSize;
}

bool packetIterator(FILE* sourceFile, int myPacketID)
{
	uint8_t frame[frameSize];
	int packetID = 0;
	int i = 0;
	while (!(packetID == myPacketID))
	{
		if(fread(frame, sizeof(frame), 1, sourceFile) == 1)
		{
			packetID = static_cast<int>(frame[1] & 0x1F)*0x100 + (int)frame[2];
			i++;
		}
		else
		{
			return false;
		} 
	}
		int stopFlag = static_cast<int>(frame[1] & 0x40);
		fseek (sourceFile , -frameSize * i , SEEK_CUR);
		if (stopFlag == 0x40)
		{
			return true;
		} 
		else 
		{
			return false;
		}
}

uint8_t TS_Header(uint8_t xTS[], FILE* sourceFile, int myTS_id)
{
	
	uint16_t AF = static_cast<int>(xTS[3] & 0x30)/0x10;
	uint16_t S = static_cast<int>((xTS[1] >> 6) & 0x1);
	uint16_t PID = static_cast<int>(xTS[1] & 0x1F)*0x100 + static_cast<int>(xTS[2]);
	uint16_t AFL = static_cast<int>(xTS[4]);
	cout << setfill('0') << setw(9) << (myTS_id) << " TS: ";
	cout << " SB=" << static_cast<int>(xTS[0]);
	cout << " E=" << static_cast<int>((xTS[1] >> 7) & 0x01);
	cout << " S=" << static_cast<int>((xTS[1] >> 6) & 0x01);
	cout << " P=" << static_cast<int>((xTS[1] >> 5) & 0x01);
	cout << " PID=" << setfill(' ') << setw(4) << static_cast<int>(xTS[1] & 0x1F)*0x100 + static_cast<int>(xTS[2]);
	cout << " TSC=" << static_cast<int>(xTS[3] & 0xC0)/0x40;
	cout << " AF=" << static_cast<int>(xTS[3] & 0x30)/0x10;
	cout << " CC=" << setfill(' ') << setw(2) << static_cast<int>(xTS[3] & 0x0F);
	if (AF == 2 or AF == 3){
		TS_AdaptationField(xTS, AF);
	}
	return 0;
}

uint8_t TS_AdaptationField(uint8_t xTS[], int myAF)
{
		
		uint8_t mask=1;
		cout << " AFL="<< static_cast<int>(xTS[4]);
		cout << " DC=" << static_cast<int>((xTS[5] >> 7) & mask);
		cout << " RA=" << static_cast<int>((xTS[5] >> 6) & mask);
		cout << " SP=" << static_cast<int>((xTS[5] >> 5) & mask);
		cout << " PR=" << static_cast<int>((xTS[5] >> 4) & mask);
		cout << " OR=" << static_cast<int>((xTS[5] >> 3) & mask);
		cout << " SP=" << static_cast<int>((xTS[5] >> 2) & mask);
		cout << " TP=" << static_cast<int>((xTS[5] >> 1) & mask);
		cout << " EX=" << static_cast<int>((xTS[5] >> 0) & mask);
		
		uint8_t AFL = static_cast<int>(xTS[4]);// & mask
		uint8_t PR = static_cast<int>((xTS[5] >> 4) & mask);
		uint8_t OR = static_cast<int>((xTS[5] >> 3) & mask);
		if (PR == 1){
			uint64_t programClockRefMain;
			uint64_t programClockRefSupp;
			string PCR_01 = "";
			
			for (int i = 0; i < 6; i++){
				for (int j = 7; j >= 0; j--){
					if (((xTS[6+i] >> j) & mask))
					PCR_01 += "1";
					else PCR_01 += "0";
				}
			}
			bitset<64> convertString (PCR_01);//Tworzymy liczbe z bitow utworzonych z stringa
			programClockRefMain = ((convertString>>0xF).to_ullong())*0x12C;
			programClockRefSupp = (convertString &= 0x1FF).to_ullong();
			uint64_t finalPCR = programClockRefMain+ programClockRefSupp;//Program Clock Reference
			cout << " PCR=" << finalPCR << " ";
			cout << "(Time=" <<fixed<<setprecision(6)<<static_cast<float>(finalPCR/27000000.0) << "s) ";//27.0 MHz
		}
		if (OR == 1){
			uint64_t OPCRMain;
			uint64_t OPCRSupp;
			string OPCR_01 = "";
			
			for (int i = 0; i < 6; i++)
			{
				for (int j = 7; j >= 0; j--)
				{
					if ((((xTS[6*2+i] >> j) & mask)))
					OPCR_01 += "1";
					else OPCR_01 += "0";
				}
			}
			bitset<64> convertString (OPCR_01); //Tworzymy liczbe z bitow utworzonych z stringa
			OPCRMain = (convertString>>0xF).to_ullong()*0x12C;
			OPCRSupp = (convertString &= 0x1FF).to_ullong();
			uint64_t finalOPCR = OPCRMain + OPCRSupp;
			cout << "OPCR=" << finalOPCR << " ";
			cout << "(Time=" <<fixed<<setprecision(6)<<static_cast<float>(finalOPCR/27000000.0) << "s) ";//27.0 MHz
		} else if (OR == 1 && PR == 1){
			uint64_t programClockRefMain;
			uint64_t programClockRefSupp;
			string PCR_01 = "";
			
			for (int i = 0; i < 6; i++)
			{
				for (int j = 7; j >= 0; j--)
				{
					if ((((xTS[6+i] >> j) & 0x01)))
					PCR_01 += "1";
					else PCR_01 += "0";
				}
			}
			bitset<64> convertString(PCR_01);
			programClockRefMain = (convertString>>0xF).to_ullong()*0x12C;
			programClockRefSupp = (convertString &= 0x1FF).to_ullong();
			uint64_t finalPCR = programClockRefMain + programClockRefSupp;
			cout << "PCR=" << finalPCR << " ";
			cout << "(Time=" <<fixed<<setprecision(6)<<static_cast<float>(finalPCR/27000000.0) << "s) ";//27.0 MHz
			
			uint64_t OPCRMain;
			uint64_t OPCRSupp;
			string OPCR_01 = "";
			
			for (int i = 0; i < 6; i++){
				for (int j = 7; j >= 0; j--){
					if ((((xTS[6*2+i] >> j) & 0x01)))
					OPCR_01 += "1";
					else OPCR_01 += "0";
				}
			}
			bitset<64> convertString2(OPCR_01);
			OPCRMain = (convertString2>>0x1F).to_ullong()*0x12C;
			OPCRSupp = (convertString2 &= 0x1FF).to_ullong();
			uint64_t finalOPCR = OPCRMain + OPCRSupp;
			cout << "OPCR=" << finalOPCR << " ";
			cout << "(Time=" <<fixed<<setprecision(6)<<static_cast<float>(finalOPCR/27000000.0) << "s) ";//27.0 MHz
		}
		if (myAF == 2) cout << " Stuffing=" << frameSize-5 << " ";
		else if (AFL > 1 && (PR == 1 or OR == 1))cout << " Stuffing=" << AFL-7 << " ";
		else if (AFL > 1 && PR == 0 && OR == 0) cout << " Stuffing=" << AFL-1 << " ";
		return 0;
}

uint8_t TS_PTS(uint8_t xTS[], int idAF, int off)
{	
	uint8_t mask=1;
	uint64_t bigMask=0xEFFFEFFFE;
	string PTS_01 = "";
	if (idAF == 2 or idAF == 3){
		for (int i = 0; i < 5; i++){
			for (int j = 7; j >= 0; j--){
				if ((((xTS[i + off + 2] >> j) & mask)))
					PTS_01 += "1";
					else PTS_01 += "0";
			}
		}
		bitset<40> convertPTS(PTS_01);
		convertPTS = convertPTS &= bigMask;//3 zera 
		PTS_01 = convertPTS.to_string<char,std::string::traits_type,std::string::allocator_type>();
		PTS_01 = PTS_01.substr(0x4,0x3) + PTS_01.substr(0x8,0xF) + PTS_01.substr(0x18,0xF);
		bitset<33> convertedPTS(PTS_01);
		cout << "PTS=" << convertedPTS.to_ullong() << " ";
		cout << "(Time=" <<fixed<<setprecision(6)<< static_cast<float>((convertedPTS.to_ullong())/90000.0) << "s) ";//90.0 kHz		
}
	return 0;
}

uint8_t TS_DTS(uint8_t xTS[], int idAF, int off )
{	
	uint8_t mask=1;
	uint64_t bigMask=0xEFFFEFFFE;
	string DTS_01 = "";
	for (int i = 0; i < 5; i++){
			for (int j = 7; j >= 0; j--){
				if ((((xTS[i + off + 7] >> j) & mask)))
				DTS_01 += "1";
				else DTS_01 += "0";
			}
		}
	
		bitset<40> convertDTS(DTS_01);
		convertDTS = convertDTS &= 0xEFFFEFFFE;
		DTS_01 = convertDTS.to_string<char,std::string::traits_type,std::string::allocator_type>();
		DTS_01 = DTS_01.substr(0x4,0x3) + DTS_01.substr(0x8,0xF) + DTS_01.substr(0x18,0xF);
		bitset<33> convertedDTS(DTS_01);
		cout << "DTS=" << convertedDTS.to_ullong() << " ";
		cout << "(Time=" <<fixed<<setprecision(6)<<static_cast<float>((convertedDTS.to_ullong())/90000.0) << "s) ";//90.0 Khz	
		
		return 0;
}

int main(){
	
	auto start = chrono::steady_clock::now();//Początek mierzenia czasu.
	
	userInterface();//Wywołanie interfejsu z użytkownikiem
	TS_AssemblerPES audio;
	TS_AssemblerPES video;
	TS_PES myPES;

	int32_t index = 0;
	int32_t AF = 3;						
	int32_t PID;					
	//Weryfikacja otwarcia pliku
	assert(sourceFile = fopen(filename, "rb"));//rb - read binary

	if (parsingSelector == 0){
		ofstream resultAudio("PID136.mp3");
		resultAudio.close();
	} else if (parsingSelector == 1){
		ofstream resultVideo("PID174.264");
		resultVideo.close();
	}
	cout<<"\nParsing in progress...";
	while(!feof(sourceFile)){
		fread(xTS, sizeof(xTS), 1, sourceFile);
		
		PID = static_cast<int>(xTS[1] & 0x1F)*0x100 + static_cast<int>(xTS[2]);
		
		if (PID == 136 && parsingSelector == 0){	
			if (parsingMode)
			{
				TS_Header(xTS, sourceFile, index );
				myPES.TS_HeaderPES(xTS, sourceFile, PID);
			} 
			audio.absorbPacket(xTS, sourceFile);
		} 
		if (PID == 174 && parsingSelector == 1){
			if (parsingMode)
			{
				TS_Header(xTS, sourceFile, index);
				myPES.TS_HeaderPES(xTS, sourceFile, PID);
			} 
			video.absorbPacket(xTS, sourceFile);
		}
		index++;
	}
	fclose(sourceFile);
	auto end = chrono::steady_clock::now();
  	cout << "\n\nParsing successfully completed. Elapsed time: "
      << chrono::duration_cast<chrono::milliseconds>(end - start).count()//Zwracanie czasu trwania całego procesu parsowania
      << " ms";
	return 0;
}
