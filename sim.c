#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/*************************************************/
/**************** define constants ***************/
/*************************************************/

#define NUM_OF_REGISTERS 16                    /* number of registers */
#define NUM_OF_IO_REGISTERS 23                 /* number of input-output registers */
#define MAIN_MEMORY_DEPTH 4096				   /* depth of main memory (0 to 4095) */
#define MEMWORD_WIDTH_HEX 5                    /* width of a word in main and disk memory in hexadecimal digits */
#define MONITOR_WIDTH_HEX 2                    /* width of a monitor pixel in hexadecimal digits */  
#define MONITOR_PX_DIM 256                     /* monitor is 256x256 pixels */
#define DISK_SECTORS 128                       /* the number of sectors in the disk */
#define LINES_PER_SECTOR 128                   /* the number of lines for each sector in the disk memory */
#define DISK_R_W_TIME 1024                     /* the number of clock cycles it takes for the disk to finish a read/write operation */
#define MAX_LINE_SIZE 300                      /* max characters in a line of an input file */
#define MAX_OPCODE_NUM 21                      /* largest opcode number */

typedef int bool;
#define true 1
#define false 0

/* for the disk */
#define FREE 0
#define BUSY 1
#define NO_COMMAND 0
#define READ 1
#define WRITE 2
#define INTERRUPT 0
#define FINISH_READ_OR_WRITE 1

/* numbers of important registers */
#define ZERO_REG 0
#define IMM_REG 1

/* numbers of important io_registers */
#define IRQ0_ENABLE 0
#define IRQ1_ENABLE 1
#define IRQ2_ENABLE 2
#define IRQ0_STATUS 3
#define IRQ1_STATUS 4
#define IRQ2_STATUS 5
#define IRQ_HANDLER 6
#define IRQ_RETURN 7
#define CLOCK_CYCLE_COUNTER 8
#define LEDS 9
#define DISPLAY7SEG 10
#define TIMERENABLE 11
#define TIMERCURRENT 12
#define TIMERMAX 13
#define DISKCMD 14
#define DISK_SECTOR 15
#define DISK_BUFFER 16
#define DISK_STATUS 17
#define MONITOR_ADDR 20
#define MONITOR_DATA 21
#define MONITORCMD 22

/*************************************************/
/***************** functions *********************/
/*************************************************/

/* check if there is an error after opening a file. if there is, the program is terminated */
void open_file_check(char* filename, FILE* file) {
    if (file == NULL) {
        printf("An Error Has Occurred With File %s\n", filename);
        exit(1); /* terminates the program */
    }
}

/* returns 1 iff a line contains only spaces/tabs/newline (return 0 otherwise) */
int empty_line_check(char* line) {
    int i;
    for (i = 0; i < strlen(line); i++) {
        if (isspace(line[i]) == 0) {
            return 0;
        }
    }
    return 1;
}

/**************************************************************/
/******************* initialize data structures ***************/
/**************************************************************/

/* 
Initialize main memory by reading lines from memin_filename. If memin_filename has fewer than 4096
lines the rest is initialized to 0.
main_memory should point to a MAIN_MEMORY_DEPTH entries long array of char pointers. The caller must free() the lines
in main_memory which are allocated by this function.
*/
void initialize_main_memory(char** main_memory, char* memin_filename) {
    int i = 0, num_of_lines = 0;
    FILE* memin_file = NULL;
    char line_buffer[MAX_LINE_SIZE + 1];
    memin_file = fopen(memin_filename, "r");
    open_file_check(memin_filename, memin_file);
    
    /* count the number of instructions and data in the file 
       if we have more lines than the defined maximum we will ignore the last lines */
    while (fgets(line_buffer, MAX_LINE_SIZE + 1, memin_file) && num_of_lines <= MAIN_MEMORY_DEPTH) {
        if (empty_line_check(line_buffer) != 1) { // if the line is not empty add 1 to num_of_lines 
            num_of_lines++;
        }
    }
    /* reset file pointer after counting */
    rewind(memin_file); 
    
    /* fill the main_memory array */
    for (i = 0; i < MAIN_MEMORY_DEPTH; i++) {
        main_memory[i] = calloc(MEMWORD_WIDTH_HEX + 1, sizeof(char));
        /* fill main_memory with non-ziroes lines */
        if(i < num_of_lines) { 
            fscanf(memin_file, "%s\n", main_memory[i]);
        }
        /* if(i >= num_of_lines) fill main_memory with ziroes lines */
        else { 
            strcpy(main_memory[i], "00000");
        }
    }
    fclose(memin_file);
    return;
}

/* create monitor as string array of size 256*256. initially all the pixels are black
monitor should point to a MONITOR_PX_DIM * MONITOR_PX_DIM entries long array of char pointers.
The caller must free() the pixel strings which are allocated by this function. */
void initialize_monitor(char** monitor) {
    int i;
    for (i = 0; i < MONITOR_PX_DIM * MONITOR_PX_DIM; i++) {
        monitor[i] = calloc(MONITOR_WIDTH_HEX + 1, sizeof(char));
        strcpy(monitor[i], "00"); /* default pixel brightness is zero = "00" (which is black) */
    }
    return;
}

/* initialize disk (diskin), Loading the data from the diskin file
   the disk representing the data as array of strings
   disk should point to a DISK_SECTORS * LINES_PER_SECTOR entries long array of char pointers. The caller must free() the lines
   in disk which are allocated by this function.*/
void initialize_disk(char** disk, char* diskin_filename) {
    int i = 0;
    char line_buffer[MAX_LINE_SIZE + 1];
    FILE* diskin_file = NULL;
    diskin_file = fopen(diskin_filename, "r");
    open_file_check(diskin_filename, diskin_file);

    /* create disk array */
    for (i = 0; i < DISK_SECTORS * LINES_PER_SECTOR; i++) {
        disk[i] = calloc(MEMWORD_WIDTH_HEX + 1, sizeof(char));
        strcpy(disk[i], "00000"); 
    }
    /* fill disk with the data from the diskin file */
    for (i = 0; i < DISK_SECTORS * LINES_PER_SECTOR; i++) {
        fscanf(diskin_file, "%s\n", disk[i]);
    }
    fclose(diskin_file);
    return;
}

/* create irq2in_array of clock cycles in which irq2status is set to 1
 (for a single clock cycle), as set by the input file
 The array is allocated by this function based in the file length and should be freed by the caller*/
int* initialize_irq2in_array(char* irq2in_filename) {
    int i, rows_counter = 0;
    FILE* irq2in_file = NULL;
    char line_buffer[MAX_LINE_SIZE + 1];

    irq2in_file = fopen(irq2in_filename, "r");
    open_file_check(irq2in_filename, irq2in_file);

    /* count the number of non-empty lines in the file */
    while (fgets(line_buffer, MAX_LINE_SIZE + 1, irq2in_file)) {
        if (empty_line_check(line_buffer) != 1) { /* if line is not empty */
            rows_counter++;
        }
    }

    /* reset file pointer */
    rewind(irq2in_file);
    int* irq2cycles_array = calloc(rows_counter, sizeof(int));

    /* fill the array */
    for (i = 0; i < rows_counter; i++) {
        fscanf(irq2in_file, "%d\n", &irq2cycles_array[i]);
    }
    fclose(irq2in_file);
    return irq2cycles_array;
}

/* initialize zero values to both registers arrays, registers and io_registers */
void initialize_registers(int *registers, int *io_registers) {
    int i;
    for (i = 0; i < NUM_OF_REGISTERS; i++) {
        registers[i] = 0;
    }
    for (i = 0; i < NUM_OF_IO_REGISTERS; i++) {
        io_registers[i] = 0;
    }
}

/**************************************************************/
/*********************** create Output Files ******************/
/**************************************************************/

/* writes to the memout output file */
void create_memout(char** main_memory, char* memout_filename) {
    int i, last_row_index;
    FILE* memout_file = NULL;
    memout_file = fopen(memout_filename, "w");
    open_file_check(memout_filename, memout_file);

    /* create a string of "00...0" */
    char zeros_string[MEMWORD_WIDTH_HEX + 1];
    for (i = 0; i < MEMWORD_WIDTH_HEX; i++) {
        zeros_string[i] = '0';
    }
    zeros_string[i] = '\0';

    /* find the last address of non-zero data start searching from the last row */
    for (i = MAIN_MEMORY_DEPTH - 1; i >= 0; i--) {
        if (strcmp(main_memory[i], &zeros_string) != 0) {
            break;
        }
    }
    last_row_index = i;
    
    /* write the data to memout file, stop writing in the last non-zero row */
    for (i = 0; i <= last_row_index; i++) {
        fprintf(memout_file, "%s\n", main_memory[i]);
    }
    fclose(memout_file);
}

/* writes to the regout output file from the registers array (places 3-15, excluding 0-2) */
void create_regout(int *registers, char* regout_filename) {
    int i;
    FILE* regout_file = NULL;
    regout_file = fopen(regout_filename, "w");
    open_file_check(regout_filename, regout_file);

    for (i = 2; i <= 15; i++) {
        fprintf(regout_file, "%08X\n", registers[i]);
    }
    fclose(regout_file);
}

/* writes data from the disk to the diskout output file */
void create_diskout(char** disk, char* diskout_filename) {
    int i, max_row;
    FILE* diskout_file = NULL;
    diskout_file = fopen(diskout_filename, "w");
    open_file_check(diskout_filename, diskout_file);

    /* find the last address of non-zero data start serching from the last row */
    for (i = DISK_SECTORS * LINES_PER_SECTOR - 1; i >= 0; i--) {
        if (strcmp(disk[i], "00000") != 0) {
            break;
        }
    }
    max_row = i;

    /* write the data to diskout file */
    for (i = 0; i <= max_row; i++) {
        fprintf(diskout_file, "%s\n", disk[i]);
    }
    fclose(diskout_file);
}

/* writes to the monitor output file: monitor.txt */
void create_monitor_txt(char** monitor, char* monitortxt_filename) {
    int i, last_row;
    unsigned char pixel;
    FILE* monitortxt_file = NULL;
    monitortxt_file = fopen(monitortxt_filename, "w");
    open_file_check(monitortxt_filename, monitortxt_file);

    /* getting the last address of non-zero data */
    for (i = MONITOR_PX_DIM * MONITOR_PX_DIM - 1; i >= 0; i--) {
        if (strcmp(monitor[i], "00") != 0) {
            break;
        }
    }
    last_row = i;

    for (i = 0; i <= last_row; i++) {
        fprintf(monitortxt_file, "%s\n", monitor[i]);
    }
    
    fclose(monitortxt_file);
}

/* writes to the cycles output file the cycle count at the end of the run */
void create_cycles(int* clock_cycle_counter, char* cycles_filename) {
    FILE* cycles_file = NULL;
    cycles_file = fopen(cycles_filename, "w");
    open_file_check(cycles_filename, cycles_file);
    
    fprintf(cycles_file, "%d\n", clock_cycle_counter);
    fclose(cycles_file);
}

/* close the files: trace, hwregtrace, leds, display7seg
   free the strings in the memory arrays: main_memory, disk, monitor
   and finally free irq2cycles_array. */
void close_files_and_free_memory(FILE* trace_file, FILE* hwregtrace_file, FILE* leds_file, FILE* display7seg_file, char** main_memory, char** disk, char** monitor, int* irq2cycles_array) {
    /* close files */
    fclose(trace_file);
    fclose(hwregtrace_file);
    fclose(leds_file);
    fclose(display7seg_file);

    /* free the memory of all the arrays we define */
    int i;
    for (i = 0; i < MAIN_MEMORY_DEPTH; i++) { free(main_memory[i]); }
    for (i = 0; i < DISK_SECTORS * LINES_PER_SECTOR; i++) { free(disk[i]); }
    for (i = 0; i < MONITOR_PX_DIM * MONITOR_PX_DIM; i++) { free(monitor[i]); }
    free(irq2cycles_array);
}

/**************************************************************/
/**************** Strings and other Functions *****************/
/**************************************************************/

/* maps the number of an I/O register to its name. the result is stored in the input string */
void reg_io_num_to_name(int reg_io_num, char* reg_io_name) {
    reg_io_num = mod(reg_io_num, NUM_OF_IO_REGISTERS);
    switch (reg_io_num) {
    case 0:  strcpy(reg_io_name, "irq0enable"); break;
    case 1:  strcpy(reg_io_name, "irq1enable"); break;
    case 2:  strcpy(reg_io_name, "irq2enable"); break;
    case 3:  strcpy(reg_io_name, "irq0status"); break;
    case 4:  strcpy(reg_io_name, "irq1status"); break;
    case 5:  strcpy(reg_io_name, "irq2status"); break;
    case 6:  strcpy(reg_io_name, "irqhandler"); break;
    case 7:  strcpy(reg_io_name, "irqreturn"); break;
    case 8:  strcpy(reg_io_name, "clks"); break;
    case 9:  strcpy(reg_io_name, "leds"); break;
    case 10: strcpy(reg_io_name, "display7seg"); break;
    case 11: strcpy(reg_io_name, "timerenable"); break;
    case 12: strcpy(reg_io_name, "timercurrent"); break;
    case 13: strcpy(reg_io_name, "timermax"); break;
    case 14: strcpy(reg_io_name, "diskcmd"); break;
    case 15: strcpy(reg_io_name, "disksector"); break;
    case 16: strcpy(reg_io_name, "diskbuffer"); break;
    case 17: strcpy(reg_io_name, "diskstatus"); break;
    case 18: strcpy(reg_io_name, "reserved"); break;
    case 19: strcpy(reg_io_name, "reserved"); break;
    case 20: strcpy(reg_io_name, "monitoraddr"); break;
    case 21: strcpy(reg_io_name, "monitordata"); break;
    case 22: strcpy(reg_io_name, "monitorcmd"); break;
    }
}

/* calculates a (mod b) for b > 0 */
int mod(int a, int b) {
    int result;
    result = a % b;
    if (a < 0 && b > 0) {
        result += b;
    }
    return result;
}

/* converts a string of a number in hexadecimal to an integer. if the input number is negative it needs
to be sign extended properly for the output integer to be negative */
int hex_to_int(char* num_hex) {
    int num;
    sscanf(num_hex, "%X", &num);
    return num;
}

/* first it convert hexadecimal string and turns it into an integer number
   then extracts from the number all the values of the registers rd, rt, rs and opcode */
void get_registers_values_from_hex_string_instruction(char* hex_string, int* p_rt, int* p_rs, int* p_rd, int* p_opcode) {
    
   int num;
   char* rt_str[2]; 
   strncpy(rt_str, &hex_string[4], 1);
   rt_str[1] = '\0';
   sscanf(rt_str, "%X", &num);
   *p_rt = num;

   char* rs_str[2];
   strncpy(rs_str, &hex_string[3], 1);
   rs_str[1] = '\0';
   sscanf(rs_str, "%X", &num);
   *p_rs = num;
    
   char* rd_str[2];
   strncpy(rs_str, &hex_string[2], 1);
   rd_str[1] = '\0';
   sscanf(rs_str, "%X", &num);
   *p_rd = num;

   char* opcode_str[2];
   strncpy(opcode_str, &hex_string[0], 2);
   opcode_str[1] = '\0';
   sscanf(opcode_str, "%X", &num);
   *p_opcode = num;

    return;
}

/* convert 5 digit hexadecimal string (representing a word in memory) into an integer number and return the number */
int get_imm_from_memory_word(char* instruction) {
    int num;
    sscanf(instruction, "%X", &num);
    num = (num << 12) >> 12; /* sign extend immediate value */
    return num;
}

/* converts an integer (may be negative) into a hexadecimal string of 5 digits, representing a word in main or disk memory.
   If it is larger than 20 bits only the lower 20 bits are used.
   the result is stored in the num_hex buffer */
void int_to_mem_word(int num, char* num_hex) {
    sprintf(num_hex, "%05X", num & 0xfffff);
}

/* converts an integer (may be negative) into a hexadecimal string of 2 digits, representing pixel brightness in the monitor.
If it is larger than 8 bits only the lower 8 bits are used.
the result is stored in the num_hex buffer */
void int_to_monitor_word(int num, char* num_hex) {
    sprintf(num_hex, "%02X", num & 0xff);
}

/**************************************************************/
/**************** Functions for each iteration ****************/
/**************************************************************/

/* writes a single line to the output trace file */
void update_trace(int PC, char* instruction, int* registers, FILE* trace_file) {
    
    int i;
    
    /* 3 digits for PC */
    char pc_str[4];
    sprintf(pc_str, "%03X", PC);
    fprintf(trace_file, "%s ", pc_str);
    
    /* 5 digits for clock cycle */
    fprintf(trace_file, "%s ", instruction);

    /* 8 digits for eche register */
    for (i = 0; i < 15; i++) {
        fprintf(trace_file, "%08X ", registers[i]);
    }
    fprintf(trace_file, "%08X\n", registers[i]); /* for i = 15 */
}

/* writes a single line to the output hwregtrace file */
void update_hwregtrace(int* io_registers, int clock_cycle_counter, char* command, int io_reg_num, FILE* hwregtrace_file) {
    
    char io_reg_name[32];
    reg_io_num_to_name(io_reg_num, io_reg_name);
    fprintf(hwregtrace_file, "%d %s %s %08X\n", clock_cycle_counter, command, io_reg_name, io_registers[io_reg_num]);
}

bool imm_instruction(int rd, int rs, int rt) {
    return (rs == 1 || rt == 1 || rd == 1);
}


/* instruction implementations */
void add_instruction(int* registers, int rd, int rs, int rt) {
    if (rd != 0) {
        registers[rd] = registers[rs] + registers[rt];
    }
}
void sub_instruction(int* registers, int rd, int rs, int rt) {
    if (rd != 0 && rd != 1) {
        registers[rd] = registers[rs] - registers[rt];
    }
}
void mul_instruction(int* registers, int rd, int rs, int rt) {
    if (rd != 0 && rd != 1) {
        registers[rd] = registers[rs] * registers[rt];
    }
}
void and_instruction(int* registers, int rd, int rs, int rt) {
    if (rd != 0 && rd != 1) {
        registers[rd] = registers[rs] & registers[rt];
    }
}
void or_instruction(int* registers, int rd, int rs, int rt) {
    if (rd != 0 && rd != 1) {
        registers[rd] = registers[rs] | registers[rt];
    }
}
void xor_instruction(int* registers, int rd, int rs, int rt) {
    if (rd != 0 && rd != 1) {
        registers[rd] = registers[rs] ^ registers[rt];
    }
}
void sll_instruction(int* registers, int rd, int rs, int rt) {
    if (rd != 0 && rd != 1) {
        registers[rd] = registers[rs] << registers[rt];
    }
}
void sra_instruction(int* registers, int rd, int rs, int rt) {
    if (rd != 0 && rd != 1) {
        registers[rd] = registers[rs] >> registers[rt];
    }
}
void srl_instruction(int* registers, int rd, int rs, int rt) {
    if (rd != 0 && rd != 1) {
        registers[rd] = (int)((unsigned)registers[rs] >> registers[rt]);
    }
}
void beq_instruction(int* registers, int* PC, int rd, int rs, int rt) {
    if (registers[rs] == registers[rt]) {
        *PC = registers[rd];
    }
}
void bne_instruction(int* registers, int* PC, int rd, int rs, int rt) {
    if (registers[rs] != registers[rt]) {
        *PC = registers[rd];
    }
}
void blt_instruction(int* registers, int* PC, int rd, int rs, int rt) {
    if (registers[rs] < registers[rt]) {
        *PC = registers[rd];
    }
}
void bgt_instruction(int* registers, int* PC, int rd, int rs, int rt) {
    if (registers[rs] > registers[rt]) {
        *PC = registers[rd];
    }
}
void ble_instruction(int* registers, int* PC, int rd, int rs, int rt) {
    if (registers[rs] <= registers[rt]) {
        *PC = registers[rd];
    }
}
void bge_instruction(int* registers, int* PC, int rd, int rs, int rt) {
    if (registers[rs] >= registers[rt]) {
        *PC = registers[rd];
    }
}
void jal_instruction(int* registers, int* PC, int rd, int rs) {
    registers[rd] = *PC;
    *PC = registers[rs];
}
void lw_instruction(int *registers, char** main_memory, int rd, int rs, int rt, int *clock_cycle_counter) {
    int temp = registers[rs] + registers[rt];
    temp = mod(temp, MAIN_MEMORY_DEPTH); /* limiting address of data memory to be between 0 and 4095 */
    registers[rd] = hex_to_int(main_memory[temp]);
	(*clock_cycle_counter)++; /* increment cycle for memory access */
}
void sw_instruction(int* registers, char** main_memory, int rd, int rs, int rt, int *clock_cycle_counter) {
    int temp = registers[rs] + registers[rt];
    temp = mod(temp, MAIN_MEMORY_DEPTH); /* limiting address of data memory to be between 0 and 4095 */
    int_to_mem_word(registers[rd], main_memory[temp]);
	(*clock_cycle_counter)++; /* increment cycle for memory access */
}
void reti_instruction(int* io_registers, int* PC, bool* executing_ISR) {
    *PC = io_registers[7];
	*executing_ISR = false;
}
void in_instruction(int *registers, int *io_registers, int rd, int rs, int rt, int clock_cycle_counter, FILE* hwregtrace_file) {
    int sum = registers[rs] + registers[rt];
    sum = mod(sum, NUM_OF_IO_REGISTERS); /* make sure sum fits to io_registers[23] */
    registers[rd] = io_registers[sum];
    update_hwregtrace(io_registers, clock_cycle_counter, "READ", sum, hwregtrace_file);
}
void out_instruction(int *registers, int *io_registers, int rd, int rs, int rt, int clock_cycle_counter, FILE* trace_file, FILE* hwregtrace_file, FILE* leds_file, FILE* display7seg_file, char** monitor) {
    int sum = registers[rs] + registers[rt];
    sum = mod(sum, NUM_OF_IO_REGISTERS); /* make sure sum fits to io_registers[23] */
    io_registers[sum] = registers[rd];
    update_hwregtrace(io_registers, clock_cycle_counter, "WRITE", sum, hwregtrace_file);
    if (sum == LEDS) {  /* leds case */
        fprintf(leds_file, "%d %08X\n", clock_cycle_counter, io_registers[sum]); 
    }
    if (sum == DISPLAY7SEG) { /* display7seg case */
        fprintf(display7seg_file, "%d %08X\n", clock_cycle_counter, io_registers[sum]);
    }
    if (sum == MONITORCMD) { /* monitorcmd case */
        if (io_registers[sum] == 1) { /* if a pixel on the monitor is updated */
            int_to_monitor_word(io_registers[MONITOR_DATA], monitor[io_registers[MONITOR_ADDR]]); /* updates the pixel on the monitor */
        }
        io_registers[sum] = 0;
    }
    if (sum == DISKCMD) { /* diskcmd case */
        if (io_registers[sum] == 1 || io_registers[sum] == 2) { /* if diskcmd was set to read or write */
            io_registers[DISK_STATUS] = 1; /* set diskstatus as busy */
        }
    }
}

/* execute an instruction */
void execute_instruction(char** main_memory, char** disk, char** monitor, int* PC, int* registers, int* io_registers,
	int *clock_cycle_counter, bool *halt, bool* executing_ISR,
	FILE* trace_file, FILE* hwregtrace_file, FILE* leds_file, FILE* display7seg_file) {
    
    char* instruction = main_memory[*PC];

    bool is_immediate = false;
    int opcode, rd, rs, rt, imm;
    
    /* extracts from the instruction (hex in string) the values of the registers rd, rt, rs and opcode */
    get_registers_values_from_hex_string_instruction(main_memory[*PC], &rt, &rs, &rd, &opcode);
    /* make sure the registers values fit to the registers_array */
    if (rs >= NUM_OF_REGISTERS) { rs = mod(rs, NUM_OF_REGISTERS); }
    if (rt >= NUM_OF_REGISTERS) { rt = mod(rt, NUM_OF_REGISTERS); }
    if (rd >= NUM_OF_REGISTERS) { rd = mod(rd, NUM_OF_REGISTERS); }
    
    /* check if it's an isntraction with imm */
    is_immediate = imm_instruction(rd, rs, rt); 
    if (is_immediate) { /* isntraction with $imm, get the next line (the imm value) */
        imm = get_imm_from_memory_word(main_memory[*PC + 1]);
        registers[1] = imm; /* load imm to reg[1] ($imm) */
    }

    update_trace(*PC, instruction, registers, trace_file);

    /* every isntraction take at least one PC and One clock cycle
       if it's an instraction with Imm we alredy increase the PC and the clock_cycle_cunter by one */
    (*PC)++;
    (*clock_cycle_counter)++;

    if (is_immediate) { /* instruction with $imm, get the next line (the imm value) */
        (*PC)++;
        (*clock_cycle_counter)++;
    }

    /* $ziro stay 0 */
    switch (opcode) {
    case 0:  /* add */  add_instruction(registers, rd, rs, rt);  break;
    case 1:  /* sub */  sub_instruction(registers, rd, rs, rt);  break;
    case 2:  /* mul */  mul_instruction(registers, rd, rs, rt);  break;
    case 3:  /* and */  and_instruction(registers, rd, rs, rt);  break;
    case 4:  /* or */   or_instruction(registers, rd, rs, rt);   break;
    case 5:  /* xor */  xor_instruction(registers, rd, rs, rt);  break;
    case 6:  /* sll */  sll_instruction(registers, rd, rs, rt);  break;
    case 7:  /* sra */  sra_instruction(registers, rd, rs, rt);  break;
    case 8:  /* srl */  srl_instruction(registers, rd, rs, rt);  break;
    case 9:  /* beq */  beq_instruction(registers, PC, rd, rs, rt);  break;
    case 10: /* bne */  bne_instruction(registers, PC, rd, rs, rt);  break;
    case 11: /* blt */  blt_instruction(registers, PC, rd, rs, rt);  break;
    case 12: /* bgt */  bgt_instruction(registers, PC, rd, rs, rt);  break;
    case 13: /* ble */  ble_instruction(registers, PC, rd, rs, rt);  break;
    case 14: /* bge */  bge_instruction(registers, PC, rd, rs, rt);  break;
    case 15: /* jal */  jal_instruction(registers, PC, rd, rs);  break;
    case 16: /* lw */   lw_instruction(registers, main_memory, rd, rs, rt, clock_cycle_counter);   break;
    case 17: /* sw */   sw_instruction(registers, main_memory, rd, rs, rt, clock_cycle_counter);   break;
    case 18: /* reti */ reti_instruction(io_registers, PC, executing_ISR); break;
    case 19: /* in */   in_instruction(registers, io_registers, rd, rs, rt, *clock_cycle_counter, hwregtrace_file);   break;
    case 20: /* out */  out_instruction(registers, io_registers, rd, rs, rt, *clock_cycle_counter, trace_file, hwregtrace_file, leds_file, display7seg_file, monitor);  break;
    case 21: /* halt */ *halt = true; break;
    }
}

/* updates irq value and perform a jump due to irq signal if it is required */
void irq_check(int *io_registers, int *PC, bool *executing_ISR) {
    bool irq = (io_registers[IRQ0_ENABLE] & io_registers[IRQ0_STATUS]) | (io_registers[IRQ1_ENABLE] & io_registers[IRQ1_STATUS]) | (io_registers[IRQ2_ENABLE] & io_registers[IRQ2_STATUS]);
    if (irq && !(*executing_ISR)) {
        io_registers[IRQ_RETURN] = *PC;
        *PC = io_registers[IRQ_HANDLER];
		*executing_ISR = true;
    }
}

/* checks if the disk is busy reading/writing and perform a read/write operation if it is time to do so */
void disk_check(char** main_memory, char** disk, int* io_registers, int cycles_diff) {
    static int disk_copy_index = 0;
	static int disk_timer = 0;

    /* if disk is busy reading/writing */
    if (io_registers[DISK_STATUS] == BUSY) {
        /* check if disk finished writing/reading, which takes 1024 cycles */
        if (disk_timer >= DISK_R_W_TIME) {
			for (int i = 0; i < LINES_PER_SECTOR; i++)
			{
				/* read - copy chosen sector to the address of the buffer in the data memory */
				if (io_registers[DISKCMD] == READ) {
					strcpy(main_memory[io_registers[DISK_BUFFER] + i], disk[LINES_PER_SECTOR * io_registers[DISK_SECTOR] + i]);
				}
				/* write - write to the chosen sector in the disk the data saved in the address of the buffer in the data memory */
				if (io_registers[DISKCMD] == WRITE) {
					strcpy(disk[LINES_PER_SECTOR * io_registers[DISK_SECTOR] + i], main_memory[io_registers[DISK_BUFFER] + i]);
				}
			}
            disk_timer = 0;                                   /* reset timer*/
            io_registers[IRQ1_STATUS] = FINISH_READ_OR_WRITE; /* irq1status indicate the disk has finished reading/writing */
            io_registers[DISKCMD] = NO_COMMAND;               /* diskcmd set to no command */
            io_registers[DISK_STATUS] = FREE;                 /* diskstatus set to available */
        }
        /* increment the timer which counts the cycles of a disk read/write command */
        else {
            disk_timer += cycles_diff;
        }
    }
}

/* updates irq2status as set by the irq2in input file */
void irq2status_check(int* irq2cycles_array, int* io_registers, int clock_cycle_counter) {
	static irq2_index = 0;
    /* if current clock cycle is set to turn on irq2status */
    if (clock_cycle_counter >= irq2cycles_array[irq2_index]) {
        io_registers[IRQ2_STATUS] = 1;
        irq2_index += 1;
    }
}

/* if timerenable is on, updates timercurrent accordingly */
void timerenable_check(int *io_registers, int cycles_diff) {
    if (io_registers[TIMERENABLE] == 1) {  /* if timerenable is on */
        if (io_registers[TIMERCURRENT] >= io_registers[TIMERMAX]) { /* if timercurrent passed timermax */
            io_registers[TIMERCURRENT] = 0; /* reset timercurrent */
            io_registers[IRQ0_STATUS] = 1;  /* turn on irq0status */
        }
        else {
            io_registers[TIMERCURRENT] += cycles_diff; /* increment timercurrent */
        }
    }
}


/* go over instruction memory and execute the instructions of the program */
void run_program(char* memin_filename, char* diskin_filename, char* irq2in_filename, char* memout_filename, char* regout_filename, char* trace_filename, char* hwregtrace_filename,
    char* cycles_filename, char* leds_filename, char* display7seg_filename, char* diskout_filename, char* monitortxt_filename) {

    /* all the files we update every iteration initialize as NULL */
    FILE* trace_file = NULL, * hwregtrace_file = NULL, * leds_file = NULL, * display7seg_file = NULL;
    
    /* array of strings representing the main memory, disk and monitor */
    char* main_memory[MAIN_MEMORY_DEPTH];  
    char* disk[DISK_SECTORS * LINES_PER_SECTOR];
    char* monitor[MONITOR_PX_DIM * MONITOR_PX_DIM];

	/* array representing the cycles when irq2 is raised */
	int* irq2cycles_array;

	/* arrays of the regular and i/o registers */
	int registers[NUM_OF_REGISTERS], io_registers[NUM_OF_IO_REGISTERS];

	/* important status integers and booleans */
	int PC = 0, clock_cycle_counter = 0;
	bool executing_ISR = false, halt = false;

    /* load data from files: memin, diskin, irq2in and create black monitor.
       initialize the arrays: main_memory, monitor, disk, irq2in_array
       and put ziroes in registers and io_registers arrays  */
    initialize_main_memory(main_memory, memin_filename);
    initialize_disk(disk, diskin_filename);
    initialize_monitor(monitor);
    irq2cycles_array = initialize_irq2in_array(irq2in_filename);
    initialize_registers(registers, io_registers);

    /* opening (and checking) the files: trace, hwregtrace, leds, display7seg in write mode */
	trace_file = fopen(trace_filename, "w");
    open_file_check(trace_filename, trace_file);
    hwregtrace_file = fopen(hwregtrace_filename, "w");
    open_file_check(hwregtrace_filename, hwregtrace_file);
    leds_file = fopen(leds_filename, "w");
    open_file_check(leds_filename, leds_file);
    display7seg_file = fopen(display7seg_filename, "w");
    open_file_check(display7seg_filename, display7seg_file);
    
    /* only halt instruction will stop the program */
    while (!halt) {
		int clock_cycle_before = clock_cycle_counter;
        execute_instruction(main_memory, disk, monitor, &PC, registers, io_registers,
			&clock_cycle_counter, &halt, &executing_ISR,
			trace_file, hwregtrace_file, leds_file, display7seg_file);
		int cycles_diff = clock_cycle_counter - clock_cycle_before;

		irq2status_check(irq2cycles_array, io_registers, clock_cycle_counter);
		disk_check(main_memory, disk, io_registers, cycles_diff);
		timerenable_check(io_registers, cycles_diff);
		irq_check(io_registers, &PC, &executing_ISR);
        io_registers[CLOCK_CYCLE_COUNTER] = clock_cycle_counter; // updating the number of clock cycles in the designated I/O register 
    }
    
    /* create the output files: memout, regout, monitor.txt, cycles */
    create_memout(main_memory, memout_filename);
    create_regout(registers, regout_filename);
    create_diskout(disk, diskout_filename);
    create_monitor_txt(monitor, monitortxt_filename);
    create_cycles(clock_cycle_counter, cycles_filename);
   
    /* close the file: trace, hwregtrace, leds, display7seg
       free tha arrays we define: instructions_memory, memout, disk, monitor, irq2cycles_array
       free the memory of the files: memin, diskin, irq2in, memout, regout, trace, hwregtrace,
       cycles, leds, display7seg, diskout ,monitortxt */
    close_files_and_free_memory(trace_file, hwregtrace_file, leds_file, display7seg_file, main_memory, disk, monitor, irq2cycles_array);
}

 
/******* main ********/
int main(int argc, char* argv[]) {
    
    char* memin_filename, * diskin_filename, * irq2in_filename, * memout_filename, * regout_filename, * trace_filename, * hwregtrace_filename,
        * cycles_filename, * leds_filename, * display7seg_filename, * diskout_filename, * monitortxt_filename;

    if (argc == 13) {
        memin_filename = argv[1];
        diskin_filename = argv[2];
        irq2in_filename = argv[3];
        memout_filename = argv[4];
        regout_filename = argv[5];
        trace_filename = argv[6];
        hwregtrace_filename = argv[7];
        cycles_filename = argv[8];    
        leds_filename = argv[9];      
        display7seg_filename = argv[10]; 
        diskout_filename = argv[11];     
        monitortxt_filename = argv[12]; 
        run_program(memin_filename, diskin_filename, irq2in_filename, memout_filename, regout_filename, trace_filename, hwregtrace_filename,
            cycles_filename, leds_filename, display7seg_filename, diskout_filename, monitortxt_filename);
    }
    /* number of command line input arguments is invalid */
    else {
        int i = 1;
        printf("Invalid Input Arguments\n");
        return 1;
    }
    
    return 0;
}
