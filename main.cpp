#include <iostream>
#include <fstream>
using namespace std;

//---------------------------------------------------------------------------

// размер файла
long filesize(FILE *stream)
{
	long curpos, length;
	curpos = ftell(stream);
	fseek(stream, 0L, SEEK_END);
	length = ftell(stream);
	fseek(stream, curpos, SEEK_SET);
	return length;
}

// функция, реализующая работу ГОСТ 28147-89 в режиме простой замены
void modeSimpRep(int mode, char* fnameIn, char* fnameOut)
{
	FILE *f_begin, *f_end; // исходный и выходной файлы 
	//таблица замен
	unsigned char RepTable[8][16] =
	{
		0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
		0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
		0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
		0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
		0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
		0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
		0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
		0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF
	};

	unsigned long key[8] =
	{
		0x0123,
		0x4567,
		0x89AB,
		0xCDEF,
		0x0123,
		0x4567,
		0x89AB,
		0xCDEF
	};

	char N[4]; // 32-разрядный накопитель,

	unsigned long N1=0, N2=0, SUM232=0; // накопители N1, N2, и сумматор

	fopen_s(&f_begin, fnameIn,"rb");
	fopen_s (&f_end, fnameOut,"wb");

	// количество блоков
	float count_bloks;
	count_bloks = 8*filesize(f_begin)/64.0;
	int block = count_bloks;
	if (count_bloks-block>0) 
		block++;

	int sh;
	if (filesize(f_begin)>=4) 
		sh = 4; 
	else sh = filesize(f_begin);
	int sh1 = 0;
	int flag=0;

	// считывание и преобразование блоков
	for (int i=0; i<block; i++)
	{
		// записываем в накопитель N1
		for (int q=0; q<4; q++) *((unsigned char*)&N+q) = 0x00;
		if ((sh1+sh)<filesize(f_begin))
		{
			fread(N,sh,1,f_begin);
			sh1+=sh;
		}
		else
		{
			sh=filesize(f_begin)-sh1;
			fread (N,sh,1,f_begin);
			flag=1;
		}
		N1 = *((unsigned long *)&N);
		// записываем в накопитель N2
		for (int q=0; q<4; q++) *((unsigned char*)&N+q) = 0x00;
		if ((sh1+sh)<filesize(f_begin))
		{
			fread (N,sh,1,f_begin);
			sh1+=sh;
		}
		else
		{
			if (flag==0)
			{
				sh=filesize(f_begin)-sh1;
				fread (N,sh,1,f_begin);
			} 
		}
		N2 = *((unsigned long *)&N);

		// 32 цикла простой замены
		int c = 0;
		for (int k=0; k<32; k++)
		{
			if (mode==1) 
				{
					if (k==24) 
						c = 7;
				}
			else if (k==8) 
				c = 7;

			// суммируем в сумматоре СМ1
			SUM232 = key[c] + N1;

			// заменяем по таблице замен
			unsigned char first_byte=0,second_byte=0,rep_symbol=0;
			int n = 7;
			for (int q=3; q>=0; q--)
			{
				rep_symbol = *((unsigned char*)&SUM232+q);
				first_byte = (rep_symbol & 0xF0) >> 4;
				second_byte = (rep_symbol & 0x0F);
				first_byte = RepTable[n][first_byte];
				n--;
				second_byte = RepTable[n][second_byte];
				n--;
				rep_symbol = (first_byte << 4) | second_byte;
				*((unsigned char*)&SUM232+q) = rep_symbol;
			} 

			SUM232 = (SUM232<<11)|(SUM232>>21); // циклический сдвиг на 11
			SUM232 = N2^SUM232; // складываем в сумматоре СМ2
			if (k<31)
			{
				N2 = N1;
				N1 = SUM232;
			}

			if (mode == 1)
			{
				if (k<24)
				{
					c++;
					if (c>7) c = 0;
				}
				else
				{
					c--;
					if (c<0) c = 7;
				}
			}
			else
			{
				if (k<8)
				{
					c++;
					if (c>7) c = 0;
				}
				else
				{
					c--;
					if (c<0) c = 7;
				}
			}
		}
		N2 = SUM232;

		// вывод результата в файл
		char sym_rez;
		for (int q=0; q<=3; q++)
		{
			sym_rez = *((unsigned char*)&N1+q);
			fprintf(f_end, "%c", sym_rez);
		}
		for (int q=0; q<=3; q++)
		{
			sym_rez = *((unsigned char*)&N2+q);
			fprintf(f_end, "%c", sym_rez);
		}
	}
	fclose (f_begin);
	fclose (f_end);
}

//---------------------------------------------------------------------------

int main(int argc, char** argv)
{
	// выбираем шифрование или расшифрование
	int menu = 0;
	setlocale(LC_ALL, "rus");
	do
	{
		system("cls");
		cout<<"ГОСТ28147-89\nВыберите действие:\n1 - зашифровать файл\n2 - расшифровать файл\n";
		cin>> menu;
	} while ((menu!=1)&&(menu!=2));

	char fnameIn[50], fnameOut[50];
	cout<<"\nВведите имя исходного файла\n";
	cin>>fnameIn;
	cout<<"\nВведите имя выходного\n";
	cin>>fnameOut;
	modeSimpRep(menu, fnameIn, fnameOut); // запускаем РПЗ
	return 0;
}
