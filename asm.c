#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_LINE_SIZE 300
#define MAX_LABEL_SIZE 50

// struct defining a label mapping from name to address
typedef struct {
	char name[MAX_LABEL_SIZE + 1];
	int address;
} label;

// the possible line types
typedef enum {
	LABEL,
	WORD,
	INSTRUCTION,
	BLANK
} line_type;

/*
 * Receives an opcode name as string and returns its representation as a string with two hex digits.
 * The returned string does not need to be freed.
 */
char * opcode_name_to_hex_string(char * opcode_name)
{
	if (strcmp(opcode_name, "add") == 0)
	{
		return "00";
	}
	if (strcmp(opcode_name, "sub") == 0)
	{
		return "01";
	}
	if (strcmp(opcode_name, "mul") == 0)
	{
		return "02";
	}
	if (strcmp(opcode_name, "and") == 0)
	{
		return "03";
	}
	if (strcmp(opcode_name, "or") == 0)
	{
		return "04";
	}
	if (strcmp(opcode_name, "xor") == 0)
	{
		return "05";
	}
	if (strcmp(opcode_name, "sll") == 0)
	{
		return "06";
	}
	if (strcmp(opcode_name, "sra") == 0)
	{
		return "07";
	}
	if (strcmp(opcode_name, "srl") == 0)
	{
		return "08";
	}
	if (strcmp(opcode_name, "beq") == 0)
	{
		return "09";
	}
	if (strcmp(opcode_name, "bne") == 0)
	{
		return "0A";
	}
	if (strcmp(opcode_name, "blt") == 0)
	{
		return "0B";
	}
	if (strcmp(opcode_name, "bgt") == 0)
	{
		return "0C";
	}
	if (strcmp(opcode_name, "ble") == 0)
	{
		return "0D";
	}
	if (strcmp(opcode_name, "bge") == 0)
	{
		return "0E";
	}
	if (strcmp(opcode_name, "jal") == 0)
	{
		return "0F";
	}
	if (strcmp(opcode_name, "lw") == 0)
	{
		return "10";
	}
	if (strcmp(opcode_name, "sw") == 0)
	{
		return "11";
	}
	if (strcmp(opcode_name, "halt") == 0)
	{
		return "12";
	}
	else // should not be reached if input is valid
	{
		return "12";
	}
}

/*
* Receives a register name as string and returns its representation as a hex digit char.
*/
char register_name_to_hex_char(char * register_name)
{
	if (strcmp(register_name, "$zero") == 0)
	{
		return '0';
	}
	if (strcmp(register_name, "$imm") == 0)
	{
		return '1';
	}
	if (strcmp(register_name, "$v0") == 0)
	{
		return '2';
	}
	if (strcmp(register_name, "$a0") == 0)
	{
		return '3';
	}
	if (strcmp(register_name, "$a1") == 0)
	{
		return '4';
	}
	if (strcmp(register_name, "$a2") == 0)
	{
		return '5';
	}
	if (strcmp(register_name, "$a3") == 0)
	{
		return '6';
	}
	if (strcmp(register_name, "$t0") == 0)
	{
		return '7';
	}
	if (strcmp(register_name, "$t1") == 0)
	{
		return '8';
	}
	if (strcmp(register_name, "$t2") == 0)
	{
		return '9';
	}
	if (strcmp(register_name, "$s0") == 0)
	{
		return 'A';
	}
	if (strcmp(register_name, "$s1") == 0)
	{
		return 'B';
	}
	if (strcmp(register_name, "$s2") == 0)
	{
		return 'C';
	}
	if (strcmp(register_name, "$gp") == 0)
	{
		return 'D';
	}
	if (strcmp(register_name, "$sp") == 0)
	{
		return 'E';
	}
	if (strcmp(register_name, "$ra") == 0)
	{
		return 'F';
	}
	else // should not be reached if input is valid
	{
		return 'F';
	}
}

/*
* Converts immediate value string into a 5-digit hex string.
* Supports decimal and hex formats as well as label conversion (label_array and label_count are needed for this purpose)
* The returned string must be freed by the caller.
* Returns NULL on error (not enough memory)
*/
char * immediate_value_to_hex(char * imm, label * label_array, int label_count)
{
	char * hex_string = calloc(6, sizeof(char));
	if (hex_string == NULL)
	{
		return NULL;
	}

	long imm_int = 0;

	if (isalpha(imm[0]))
	{
		for (int i = 0; i < label_count; i++)
		{
			if (strcmp(imm, label_array[i].name) == 0)
			{
				imm_int = label_array[i].address;
				break;
			}
		}
	}
	else
	{
		imm_int = strtol(imm, NULL, 0);
	}

	// & with 0xfffff limits to 5 digits (especially relevant for negative numbers)
	// and %05X format pads with zeroes if shorter than 5 digits
	sprintf_s(hex_string, 6, "%05X", imm_int & 0xfffff); 
	return hex_string;
}

/*
* Receives an assembly line as string and determines the line type
*/
line_type get_line_type(char * line)
{
	int found_alpha = 0;
	for (size_t i = 0; i < strlen(line); i++)
	{
		if (line[i] == '#') // stop reading once we reach a comment
		{
			break;
		}
		if (line[i] == ':' && found_alpha) // : after at least one alphabetic character should indicate a label, if input is valid
		{
			return LABEL;
		}
		if (line[i] == '.' && !found_alpha) // a . before any alphabetic character should only be found in a .word instruction if input is valid
		{
			return WORD;
		}
		if (isalpha(line[i]))
		{
			found_alpha = 1;
		}
	}
	return found_alpha ? INSTRUCTION : BLANK; // in valid input if we found an alphabetic character before the end and no : it's an instruction, 
												// otherwise it's a blank line (or purely comment)
}

/*
* Reads the file to determine how many labels are in it. Rewinds the file to the start afterwards.
*/
int get_label_count(FILE * program_file)
{
	char line_buffer[MAX_LINE_SIZE + 1];
	int label_count = 0;

	while (fgets(line_buffer, MAX_LINE_SIZE + 1, program_file))
	{
		if (get_line_type(line_buffer) == LABEL)
		{
			label_count++;
		}
	}

	rewind(program_file);
	return label_count;
}

/*
* Reads the file to find all the labels and create an array or label structs representing a mapping from label names to addresses.
* Label count should already be known (by get_label_count) and passed in label_count.
* Rewinds the file to the start afterward.
* The returned array must be freed by the caller.
* Returns NULL on error (not enough memory)
*/
label * get_labels(FILE * program_file, int label_count)
{
	char line_buffer[MAX_LINE_SIZE + 1];
	label * label_array = calloc(label_count, sizeof(label));
	if (label_array == NULL)
	{
		return NULL;
	}

	label * curr_label = label_array;
	char * label_name;
	int curr_address = 0;
	char * rd;
	char * rs;
	char * rt;
	char * tok_state;

	while (fgets(line_buffer, MAX_LINE_SIZE + 1, program_file))
	{
		switch (get_line_type(line_buffer))
		{
		case INSTRUCTION:
			// tokenize the line to find the registers used and figure out if it's I-type (i.e. if $imm appears)
			strtok_s(line_buffer, " \t", &tok_state); // skip the first token (opcode name)
			rd = strtok_s(NULL, " \t,", &tok_state);
			rs = strtok_s(NULL, " \t,", &tok_state);
			rt = strtok_s(NULL, " \t,", &tok_state);
			// if I-type ($imm appears in any register) add 2 for its 2 lines, otherwise 1
			if (strcmp(rd, "$imm") == 0 || strcmp(rs, "$imm") == 0 || strcmp(rt, "$imm") == 0)
			{
				curr_address += 2;
			}
			else
			{
				curr_address += 1;
			}
			break;
		case LABEL:
			// if input is valid this should find the label name whose start can come after whitespace and its end is marked by a colon
			label_name = strtok_s(line_buffer, " \t:", &tok_state);
			strcpy_s(curr_label->name, sizeof(curr_label->name), label_name);
			curr_label->address = curr_address;
			curr_label++;
			break;
		default:
			break; // do nothing for blank lines and .word
		}
	}

	rewind(program_file);
	return label_array;
}

/*
* Reads program_file, assembles it and outputs the resulting memory image into output_file.
* Label mapping should already known by previous calls to get_label_count and get_labels, and is passed
* in label_array and label_count.
* Returns 0 on success, 1 on error (not enough memory)
*/
int assemble_program(FILE * program_file, FILE * output_file, label * label_array, int label_count)
{
	char line_buffer[MAX_LINE_SIZE + 1];
	char * mem[4096] = { NULL };
	int curr_instruction_line = 0;
	int last_used_line = -1;
	char * opcode_name;
	char * rd;
	char * rs;
	char * rt;
	char * imm;
	char * opcode_hex;
	char * tok_state;
	char * word_address;
	long word_address_int;
	char * word_value;
	long word_value_int;
	int retval = 0;

	while (fgets(line_buffer, MAX_LINE_SIZE + 1, program_file))
	{
		switch (get_line_type(line_buffer))
		{
		case INSTRUCTION:
			// tokenize the line
			opcode_name = strtok_s(line_buffer, " \t", &tok_state);
			rd = strtok_s(NULL, " \t,", &tok_state);
			rs = strtok_s(NULL, " \t,", &tok_state);
			rt = strtok_s(NULL, " \t,", &tok_state);
			imm = strtok_s(NULL, " \t\n,#", &tok_state); // a # for comment or a line feed can come right after the immediate value

			// build the machine code line
			mem[curr_instruction_line] = calloc(6, sizeof(char));
			opcode_hex = opcode_name_to_hex_string(opcode_name);
			mem[curr_instruction_line][0] = opcode_hex[0];
			mem[curr_instruction_line][1] = opcode_hex[1];
			mem[curr_instruction_line][2] = register_name_to_hex_char(rd);
			mem[curr_instruction_line][3] = register_name_to_hex_char(rs);
			mem[curr_instruction_line][4] = register_name_to_hex_char(rt);
			mem[curr_instruction_line][5] = '\0';

			curr_instruction_line++;

			// if I-type ($imm in one or more of the registers) write the immediate value in another line
			if (strcmp(rd, "$imm") == 0 || strcmp(rs, "$imm") == 0 || strcmp(rt, "$imm") == 0)
			{
				mem[curr_instruction_line] = immediate_value_to_hex(imm, label_array, label_count);
				if (mem[curr_instruction_line] == NULL)
				{
					retval = 1;
					goto cleanup;
				}
				curr_instruction_line++;
			}

			if (curr_instruction_line - 1 > last_used_line)
			{
				last_used_line = curr_instruction_line - 1;
			}
			break;
		case WORD:
			// tokenize the line
			strtok_s(line_buffer, " \t", &tok_state); // skip first token (should be .word)
			word_address = strtok_s(NULL, " \t", &tok_state);
			word_value = strtok_s(NULL, " \t\n#", &tok_state); // a # for comment or a line feed can come right after the value
			word_address_int = strtol(word_address, NULL, 0);
			word_value_int = strtol(word_value, NULL, 0);

			mem[word_address_int] = calloc(6, sizeof(char));
			if (mem[word_address_int] == NULL)
			{
				retval = 1;
				goto cleanup;
			}

			// prints exactly 5 digits similarly to immediate_value_to_hex
			sprintf_s(mem[word_address_int], 6, "%05X", word_value_int & 0xfffff);
			if (word_address_int > last_used_line)
			{
				last_used_line = word_address_int;
			}
			break;
		default:
			break; // do nothing for blank lines and labels
		}
	}

	for (int i = 0; i <= last_used_line; i++)
	{
		if (mem[i] == NULL)
		{
			fprintf_s(output_file, "00000\n");
		}
		else
		{
			fprintf_s(output_file, "%s\n", mem[i]);
		}
	}

cleanup:
	for (int i = 0; i <= last_used_line; i++)
	{
		if (mem[i] != NULL)
		{
			free(mem[i]);
		}
	}
	return retval;
}

int main(int argc, char * argv[])
{
	if (argc == 3)
	{
		char * program_filename = argv[1];
		char * output_filename = argv[2];
		FILE * program_file;
		FILE * output_file;

		if (fopen_s(&program_file, program_filename, "r") != 0)
		{
			printf("error opening input file\n");
			return 1;
		}

		if (fopen_s(&output_file, output_filename, "w") != 0)
		{
			printf("error opening input file\n");
			return 1;
		}

		int label_count = get_label_count(program_file);
		label * label_array = get_labels(program_file, label_count);
		if (label_array == NULL)
		{
			printf("memory error\n");
			fclose(program_file);
			fclose(output_file);
			return 1;
		}

		if (assemble_program(program_file, output_file, label_array, label_count) != 0)
		{
			printf("memory error\n");
			free(label_array);
			fclose(program_file);
			fclose(output_file);
			return 1;
		}

		free(label_array);
		fclose(program_file);
		fclose(output_file);
	}
	else
	{
		printf("invalid arguments\n");
		return 1;
	}
    return 0;
}

