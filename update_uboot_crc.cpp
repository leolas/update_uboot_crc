#include <stdio.h>
#include <windows.h>

char * msg = "-> Calculating crc, Input size = 0x%x\n";

int __declspec(naked) __cdecl generate_crc(BYTE* buff, int len)
{
	__asm
	{
				 push    esi
                 mov     esi, [esp+0Ch]
                 push    edi
                 push    esi
                 push    msg
                 call    printf
                 mov     edi, [esp+14h]
                 add     esp, 8
                 shr     esi, 2
                 mov     dword ptr [edi+0Ch], 5F0A6C39h
                 mov     edx, esi
                 xor     eax, eax
                 mov     ecx, edi

 loc_100048E8:                           ; CODE XREF: calc_crc_100048C0+4Aj
                 mov     esi, [ecx]
                 add     ecx, 4
                 add     eax, esi
                 sub     edx, 4
                 mov     esi, [ecx]
                 add     ecx, 4
                 add     eax, esi
                 mov     esi, [ecx]
                 add     ecx, 4
                 add     eax, esi
                 mov     esi, [ecx]
                 add     ecx, 4
                 add     eax, esi
                 cmp     edx, 3
                 ja      short loc_100048E8
                 mov     esi, edx
                 dec     edx
                 test    esi, esi
                 jbe     short loc_1000491E
                 inc     edx

 loc_10004914:                           ; CODE XREF: calc_crc_100048C0+5Cj
                 mov     esi, [ecx]
                 add     ecx, 4
                 add     eax, esi
                 dec     edx
                 jnz     short loc_10004914

 loc_1000491E:                           ; CODE XREF: calc_crc_100048C0+51j
                 mov     [edi+0Ch], eax
                 pop     edi
                 ;xor     eax, eax
                 pop     esi
                 retn
	}

}


int main(int argc, char *argv[])
{
	 struct boot_file_head
	{
			DWORD  jump_instruction;   // one intruction jumping to real code
			char   magic[8];           // ="eGON.BT0" or "eGON.BT1",  not C-style string.
			DWORD  check_sum;          // generated by PC
			DWORD  length;             // generated by PC
			DWORD  pub_head_size;      // the size of boot_file_head_t
			BYTE   pub_head_vsn[4];    // the version of boot_file_head_t
			BYTE   file_head_vsn[4];   // the version of boot0_file_head_t or boot1_file_head_t
			BYTE   Boot_vsn[4];        // Boot version
			BYTE   eGON_vsn[4];        // eGON version
			BYTE   platform[8];        // platform information
	};
	printf("U-BOOT.FEX CRC/ Config.fex updater by lolet\n\n");
	if(argc < 2)
	{
	   printf("Usage: %s <input_file> [config.fex]\n", argv[0]);
	   return EXIT_FAILURE;
	}
	
		FILE* file = fopen(argv[1], "rb+");
        if(file == 0) {
                perror("Cannot open input file");
                return EXIT_FAILURE;
        }
		FILE * config = 0;
		if(argc>2) 
		{
			config = fopen(argv[2], "rb");
			if(config == 0) {
					perror("Cannot open config file");
					return EXIT_FAILURE;
			}
		}
		
		fseek (file, 0, SEEK_END);
		int size = ftell (file);
		rewind(file);
		printf("-> File size is 0x%X\n", size);
		BYTE* img=  new BYTE[size];
		BYTE* config_fex = 0;
		
		 int count = fread(img, 1, size, file);
		 printf("-> Read 0x%x bytes\n", count);
		 if(count!=size)
		 {	
			printf("ERROR: Didn't read whole file!");
			delete[] img;
			fclose(file);
			return EXIT_FAILURE;
		 }
		 int oldcrc = ((boot_file_head*)img)->check_sum;
		 
		 if(config)
		 {
			printf("->Reading new config\n");
			if(strcmp(((boot_file_head*)img)->magic, "uboot"))
			{
				printf("ERROR: Wrong file signature! (%s)\n", ((boot_file_head*)img)->magic);
				delete[] img;
				fclose(file);
				return EXIT_FAILURE;
			}
			fseek (config, 0, SEEK_END);
			int config_size = ftell (config);
			rewind(config);
			
			if(config_size > 0xC000)
			{
				printf("ERROR: Config size is too big %d > %d\n",config_size, 0xc000);
				delete[] img;
				fclose(config);
				fclose(file);
				return EXIT_FAILURE;
			}
			
			config_fex = new BYTE[config_size];
						
			int config_count = fread(config_fex, 1, config_size, config);
			printf("-> Read 0x%x bytes of config\n", config_count);
			
			if(config_count != config_size)
			{	
				printf("ERROR: Didn't read whole config file!");
				delete[] img;
				delete[] config_fex;
				fclose(config);
				fclose(file);
				return EXIT_FAILURE;
			}
			
			memcpy(img + 0x94000, config_fex, config_size);
			int align = 0xC000 - config_size;
			memset(img + (size - align), 0xFF, align); 
		 }
		 
		 int crc = generate_crc(img, count);
		 printf("-> Old CRC = 0x%X, new CRC is 0x%X\n", oldcrc, crc);
		 if(!config)
		 {
			fseek(file, 12, SEEK_SET);
			int w = fwrite((void*)&crc, 1, 4, file);
			printf("-> Successfully written 0x%X bytes of CRC!", w);
		 }
		 else
		 {
			rewind(file);
			int w = fwrite(img, 1, size, file);
			printf("-> Successfully written 0x%X bytes of file!", w);
		 }
		 delete[] img;
		 fclose(file);
		 
		 if(config_fex)
			delete[] config_fex;
		 if(config)
		 fclose(config);

	return 0;
}