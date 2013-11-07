/*
 *  VlogHammer -- A Verilog Synthesis Regression Test
 *
 *  Copyright (C) 2013  Clifford Wolf <clifford@clifford.at>
 *  
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define GENERATE_BINARY_OPS
#define GENERATE_UNARY_OPS
#define GENERATE_TERNARY_OPS
#define GENERATE_CONCAT_OPS
#define GENERATE_REPEAT_OPS
#define GENERATE_EXPRESSIONS

// Use 'make gen_samples'
// #define ONLY_SAMPLES

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string>

const char *arg_types[][3] = {
	{ "{dir} [3:0] {name}", "{name}", "4" },	// 00
	{ "{dir} [4:0] {name}", "{name}", "5" },	// 01
	{ "{dir} [5:0] {name}", "{name}", "6" },	// 02
	{ "{dir} signed [3:0] {name}", "{name}", "4" },	// 03
	{ "{dir} signed [4:0] {name}", "{name}", "5" },	// 04
	{ "{dir} signed [5:0] {name}", "{name}", "6" }	// 05
};

const char *small_arg_types[][3] = {
	{ "{dir} [0:0] {name}", "{name}", "1" },	// 00
	{ "{dir} [1:0] {name}", "{name}", "2" },	// 01
	{ "{dir} [2:0] {name}", "{name}", "3" },	// 02
	{ "{dir} signed [0:0] {name}", "{name}", "1" },	// 03
	{ "{dir} signed [1:0] {name}", "{name}", "2" },	// 04
	{ "{dir} signed [2:0] {name}", "{name}", "3" },	// 05
};

// See Table 5-1 (page 42) in IEEE Std 1364-2005
// for a list of all Verilog oprators.

const char *binary_ops[] = {
	"+",	// 00
	"-",	// 01
	"*",	// 02
	"/",	// 03
	"%",	// 04
	"**",	// 05
	">",	// 06
	">=",	// 07
	"<",	// 08
	"<=",	// 09
	"&&",	// 10
	"||",	// 11
	"==",	// 12
	"!=",	// 13
	"===",	// 14
	"!==",	// 15
	"&",	// 16
	"|",	// 17
	"^",	// 18
	"^~",	// 19
	"<<",	// 20
	">>",	// 21
	"<<<",	// 22
	">>>",	// 23
};

const char *unary_ops[] = {
	"+",	// 00
	"-",	// 01
	"!",	// 02
	"~",	// 03
	"&",	// 04
	"~&",	// 05
	"|",	// 06
	"~|",	// 07
	"^",	// 08
	"~^",	// 09
};

#define SIZE(_list) int(sizeof(_list) / sizeof(*_list))

void strsubst(std::string &str, const std::string &match, const std::string &replace)
{
	size_t pos;
	while ((pos = str.find(match)) != std::string::npos)
		str.replace(pos, match.size(), replace);
}

uint32_t xorshift32(uint32_t seed = 0) {
	static uint32_t x = 314159265;
	if (seed)
		x = seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}

void print_expression(FILE *f, int budget, uint32_t mask, bool avoid_undef, bool avoid_signed, bool in_param)
{
	bool avoid_mult_div_mod = false;
	int num_binary_ops = SIZE(binary_ops);
	int num_unary_ops = SIZE(unary_ops);
	int num_arg_types = SIZE(arg_types);
	int num_modes = 10;
	int i, j, mode;
	const char *p;

	assert(budget >= 0);
	if (budget == 0) {
		if (in_param)
			goto print_constant;
		char var_char;
		int var_index;
		if (!avoid_undef && (xorshift32() % 256) > (mask >> 24)) {
			var_char = 'p';
			var_index = xorshift32() % (3*num_arg_types);
		} else {
			var_char = 'a' + (xorshift32() % 2);
			var_index = xorshift32() % num_arg_types;
		}
		if (avoid_signed && (var_index % num_arg_types) >= num_arg_types/2)
			var_index -= num_arg_types/2;
		fprintf(f, "%c%d", var_char, var_index);
		return;
	}

	while ((mask & ~((~0) << num_modes)) == 0)
		mask = xorshift32() & (in_param ? ~1 : ~0);

	if ((mask & 3) != 0)
		avoid_mult_div_mod = true;

	do {
		mode = xorshift32() % num_modes;
	} while (((1 << mode) & mask) == 0);

	budget--;
	switch (mode)
	{
	case 0:
		i = 1 + xorshift32() % 3;
		fprintf(f, "{");
		for (j = 0; j < i; j++) {
			if (j)
				fprintf(f, ",");
			print_expression(f, budget / i, mask, avoid_undef, avoid_signed, in_param);
		}
		fprintf(f, "}");
		break;
	case 1:
		i = (xorshift32() % 4) + 1;
		fprintf(f, "{%d{", i);
		print_expression(f, budget / i, mask, avoid_undef, avoid_signed, in_param);
		fprintf(f, "}}");
		break;
	case 2:
		if (avoid_signed)
			fprintf(f, "$unsigned(");
		else
			fprintf(f, "%s(", xorshift32() % 3 == 0 ? "$signed" :
					xorshift32() % 2 == 0 ? "$unsigned" : "");
		print_expression(f, budget, mask, avoid_undef, false, in_param);
		fprintf(f, ")");
		break;
	case 3:
	case 4:
	case 5:
		fprintf(f, "(");
		do {
			p = binary_ops[xorshift32() % num_binary_ops];
		} while ((avoid_mult_div_mod && (!strcmp(p, "*") || !strcmp(p, "/") || !strcmp(p, "%"))) ||
				(avoid_undef && (!strcmp(p, "/") || !strcmp(p, "%"))));
		if (!strcmp(p, "===") || !strcmp(p, "!=="))
			avoid_undef = true;
		if (!strcmp(p, "**")) {
			fprintf(f, "%s'd2 %s ", arg_types[xorshift32() % num_arg_types][2], p);
			print_expression(f, budget < 3 ? std::max(budget-1, 0) : 2, mask, avoid_undef, true, in_param);
		} else
		if (!strcmp(p, "/") || !strcmp(p, "%")) {
			print_expression(f, budget < 3 ? std::max(budget-1, 0) : 2, mask, avoid_undef, avoid_signed, in_param);
			fprintf(f, "%s", p);
			print_expression(f, 0, mask, avoid_undef, avoid_signed, in_param);
		} else {
			if (!strcmp(p, "*"))
				budget = budget < 4 ? budget : 4;
			print_expression(f, budget/2, mask, avoid_undef, avoid_signed, in_param);
			fprintf(f, "%s", p);
			print_expression(f, budget/2, mask, avoid_undef, avoid_signed, in_param);
		}
		fprintf(f, ")");
		break;
	case 6:
	case 7:
		fprintf(f, "(%s", unary_ops[xorshift32() % num_unary_ops]);
		print_expression(f, budget, mask, avoid_undef, avoid_signed, in_param);
		fprintf(f, ")");
		break;
	case 8:
		fprintf(f, "(");
		print_expression(f, budget / 3, mask, avoid_undef, avoid_signed, in_param);
		fprintf(f, "?");
		print_expression(f, budget / 3, mask, avoid_undef, avoid_signed, in_param);
		fprintf(f, ":");
		print_expression(f, budget / 3, mask, avoid_undef, avoid_signed, in_param);
		fprintf(f, ")");
		break;
	case 9:
print_constant:
		fprintf(f, "(");
		i = (xorshift32() % 4) + 2;
		if (xorshift32() % 2 == 0 && !avoid_signed)
			fprintf(f, "%s%d'sd%d", xorshift32() % 2 == 0 ? "-" : "", i, xorshift32() % (1 << (i-1)));
		else
			fprintf(f, "%d'd%d", i, xorshift32() % (1 << i));
		fprintf(f, ")");
		break;
	}
}

int main()
{
	mkdir("rtl", 0777);

#ifdef GENERATE_BINARY_OPS
	for (int ai = 0; ai < SIZE(arg_types); ai++)
	for (int bi = 0; bi < SIZE(arg_types); bi++)
	for (int yi = 0; yi < SIZE(arg_types); yi++)
	for (int oi = 0; oi < SIZE(binary_ops); oi++)
	{
#ifdef ONLY_SAMPLES
		if (xorshift32() % 20)
			continue;
#endif
		std::string a_decl = arg_types[ai][0];
		strsubst(a_decl, "{dir}", "input");
		strsubst(a_decl, "{name}", "a");

		std::string b_decl = arg_types[bi][0];
		strsubst(b_decl, "{dir}", "input");
		strsubst(b_decl, "{name}", "b");

		std::string y_decl = arg_types[yi][0];
		strsubst(y_decl, "{dir}", "output");
		strsubst(y_decl, "{name}", "y");

		std::string a_ref = arg_types[ai][1];
		strsubst(a_ref, "{dir}", "input");
		strsubst(a_ref, "{name}", "a");

		std::string b_ref = arg_types[bi][1];
		strsubst(b_ref, "{dir}", "input");
		strsubst(b_ref, "{name}", "b");

		std::string y_ref = arg_types[yi][1];
		strsubst(y_ref, "{dir}", "output");
		strsubst(y_ref, "{name}", "y");

		char buffer[1024];
		snprintf(buffer, 1024, "rtl/binary_ops_%02d%02d%02d%02d.v", ai, bi, yi, oi);

		FILE *f = fopen(buffer, "w");
		fprintf(f, "module binary_ops_%02d%02d%02d%02d(a, b, y);\n", ai, bi, yi, oi);
		fprintf(f, "  %s;\n", a_decl.c_str());
		fprintf(f, "  %s;\n", b_decl.c_str());
		fprintf(f, "  %s;\n", y_decl.c_str());
		if (!strcmp(binary_ops[oi], "**"))
			fprintf(f, "  assign %s = %s%s ** %s;\n", y_ref.c_str(), arg_types[ai][2],
					strstr(arg_types[ai][0], " signed ") ? "'sd2" : "'d2", b_ref.c_str());
		else
			fprintf(f, "  assign %s = %s %s %s;\n", y_ref.c_str(),
					a_ref.c_str(), binary_ops[oi], b_ref.c_str());
		fprintf(f, "endmodule\n");
		fclose(f);
	}
#endif

#ifdef GENERATE_UNARY_OPS
	for (int ai = 0; ai < SIZE(arg_types); ai++)
	for (int yi = 0; yi < SIZE(arg_types); yi++)
	for (int oi = 0; oi < SIZE(unary_ops); oi++)
	{
#ifdef ONLY_SAMPLES
		if (xorshift32() % 20)
			continue;
#endif
		std::string a_decl = arg_types[ai][0];
		strsubst(a_decl, "{dir}", "input");
		strsubst(a_decl, "{name}", "a");

		std::string y_decl = arg_types[yi][0];
		strsubst(y_decl, "{dir}", "output");
		strsubst(y_decl, "{name}", "y");

		std::string a_ref = arg_types[ai][1];
		strsubst(a_ref, "{dir}", "input");
		strsubst(a_ref, "{name}", "a");

		std::string y_ref = arg_types[yi][1];
		strsubst(y_ref, "{dir}", "output");
		strsubst(y_ref, "{name}", "y");

		char buffer[1024];
		snprintf(buffer, 1024, "rtl/unary_ops_%02d%02d%02d.v", ai, yi, oi);

		FILE *f = fopen(buffer, "w");
		fprintf(f, "module unary_ops_%02d%02d%02d(a, y);\n", ai, yi, oi);
		fprintf(f, "  %s;\n", a_decl.c_str());
		fprintf(f, "  %s;\n", y_decl.c_str());
		fprintf(f, "  assign %s = %s %s;\n", y_ref.c_str(),
				unary_ops[oi], a_ref.c_str());
		fprintf(f, "endmodule\n");
		fclose(f);
	}
#endif

#ifdef GENERATE_TERNARY_OPS
	for (int ai = 0; ai < SIZE(small_arg_types); ai++)
	for (int bi = 0; bi < SIZE(arg_types); bi++)
	for (int ci = 0; ci < SIZE(arg_types); ci++)
	for (int yi = 0; yi < SIZE(arg_types); yi++)
	{
#ifdef ONLY_SAMPLES
		if (xorshift32() % 20)
			continue;
#endif
		if (!strcmp(small_arg_types[ai][2], "3"))
			continue;
		if (!strcmp(arg_types[bi][2], "6"))
			continue;
		if (!strcmp(arg_types[ci][2], "6"))
			continue;

		std::string a_decl = small_arg_types[ai][0];
		strsubst(a_decl, "{dir}", "input");
		strsubst(a_decl, "{name}", "a");

		std::string b_decl = arg_types[bi][0];
		strsubst(b_decl, "{dir}", "input");
		strsubst(b_decl, "{name}", "b");

		std::string c_decl = arg_types[ci][0];
		strsubst(c_decl, "{dir}", "input");
		strsubst(c_decl, "{name}", "c");

		std::string y_decl = arg_types[yi][0];
		strsubst(y_decl, "{dir}", "output");
		strsubst(y_decl, "{name}", "y");

		std::string a_ref = small_arg_types[ai][1];
		strsubst(a_ref, "{dir}", "input");
		strsubst(a_ref, "{name}", "a");

		std::string b_ref = arg_types[bi][1];
		strsubst(b_ref, "{dir}", "wire");
		strsubst(b_ref, "{name}", "b");

		std::string c_ref = arg_types[ci][1];
		strsubst(c_ref, "{dir}", "wire");
		strsubst(c_ref, "{name}", "c");

		std::string y_ref = arg_types[yi][1];
		strsubst(y_ref, "{dir}", "output");
		strsubst(y_ref, "{name}", "y");

		char buffer[1024];
		snprintf(buffer, 1024, "rtl/ternary_ops_%02d%02d%02d%02d.v", ai, bi, ci, yi);

		FILE *f = fopen(buffer, "w");
		fprintf(f, "module ternary_ops_%02d%02d%02d%02d(a, b, c, y);\n", ai, bi, ci, yi);
		fprintf(f, "  %s;\n", a_decl.c_str());
		fprintf(f, "  %s;\n", b_decl.c_str());
		fprintf(f, "  %s;\n", c_decl.c_str());
		fprintf(f, "  %s;\n", y_decl.c_str());
		fprintf(f, "  assign %s = %s ? %s : %s;\n", y_ref.c_str(),
				a_ref.c_str(), b_ref.c_str(), c_ref.c_str());
		fprintf(f, "endmodule\n");
		fclose(f);
	}
#endif

#ifdef GENERATE_CONCAT_OPS
	for (int ai = 0; ai < SIZE(small_arg_types); ai++)
	for (int bi = 0; bi < SIZE(small_arg_types); bi++)
	for (int yi = 0; yi < SIZE(arg_types); yi++)
	{
#ifdef ONLY_SAMPLES
		if (xorshift32() % 20)
			continue;
#endif
		std::string a_decl = small_arg_types[ai][0];
		strsubst(a_decl, "{dir}", "input");
		strsubst(a_decl, "{name}", "a");

		std::string b_decl = small_arg_types[bi][0];
		strsubst(b_decl, "{dir}", "input");
		strsubst(b_decl, "{name}", "b");

		std::string y_decl = arg_types[yi][0];
		strsubst(y_decl, "{dir}", "output");
		strsubst(y_decl, "{name}", "y");

		std::string a_ref = small_arg_types[ai][1];
		strsubst(a_ref, "{dir}", "input");
		strsubst(a_ref, "{name}", "a");

		std::string b_ref = small_arg_types[bi][1];
		strsubst(b_ref, "{dir}", "input");
		strsubst(b_ref, "{name}", "b");

		std::string y_ref = arg_types[yi][1];
		strsubst(y_ref, "{dir}", "output");
		strsubst(y_ref, "{name}", "y");

		char buffer[1024];
		snprintf(buffer, 1024, "rtl/concat_ops_%02d%02d%02d.v", ai, bi, yi);

		FILE *f = fopen(buffer, "w");
		fprintf(f, "module concat_ops_%02d%02d%02d(a, b, y);\n", ai, bi, yi);
		fprintf(f, "  %s;\n", a_decl.c_str());
		fprintf(f, "  %s;\n", b_decl.c_str());
		fprintf(f, "  %s;\n", y_decl.c_str());
		fprintf(f, "  assign %s = {%s, %s};\n", y_ref.c_str(), a_ref.c_str(), b_ref.c_str());
		fprintf(f, "endmodule\n");
		fclose(f);
	}
#endif

#ifdef GENERATE_REPEAT_OPS
	for (int a = 0; a < 4; a++)
	for (int bi = 0; bi < SIZE(small_arg_types); bi++)
	for (int yi = 0; yi < SIZE(arg_types); yi++)
	{
#ifdef ONLY_SAMPLES
		if (xorshift32() % 20)
			continue;
#endif
		std::string b_decl = small_arg_types[bi][0];
		strsubst(b_decl, "{dir}", "input");
		strsubst(b_decl, "{name}", "b");

		std::string y_decl = arg_types[yi][0];
		strsubst(y_decl, "{dir}", "output");
		strsubst(y_decl, "{name}", "y");

		std::string b_ref = small_arg_types[bi][1];
		strsubst(b_ref, "{dir}", "input");
		strsubst(b_ref, "{name}", "b");

		std::string y_ref = arg_types[yi][1];
		strsubst(y_ref, "{dir}", "output");
		strsubst(y_ref, "{name}", "y");

		char buffer[1024];
		snprintf(buffer, 1024, "rtl/repeat_ops_%02d%02d%02d.v", a, bi, yi);

		FILE *f = fopen(buffer, "w");
		fprintf(f, "module repeat_ops_%02d%02d%02d(b, y);\n", a, bi, yi);
		fprintf(f, "  %s;\n", b_decl.c_str());
		fprintf(f, "  %s;\n", y_decl.c_str());
		fprintf(f, "  assign %s = {%d{%s}};\n", y_ref.c_str(), a, b_ref.c_str());
		fprintf(f, "endmodule\n");
		fclose(f);
	}
#endif

#ifdef GENERATE_EXPRESSIONS
	for (int i = 0; i < 1000; i++)
	{
#ifdef ONLY_SAMPLES
		if (i == 100)
			break;
#endif
		xorshift32(1234 + i);
		xorshift32();
		xorshift32();
		xorshift32();

		char buffer[1024];
		snprintf(buffer, 1024, "rtl/expression_%05d.v", i);

		FILE *f = fopen(buffer, "w");
		fprintf(f, "module expression_%05d(", i);

		for (char var = 'a'; var <= 'b'; var++)
		for (int j = 0; j < SIZE(arg_types); j++)
			fprintf(f, "%c%d, ", var, j);
		fprintf(f, "y);\n");

		for (char var = 'a'; var <= 'y'; var++) {
			for (int j = 0; j < SIZE(arg_types)*(var == 'y' ? 3 : 1); j++) {
				std::string decl = arg_types[j % SIZE(arg_types)][0];
				strsubst(decl, "{dir}", var == 'y' ? "wire" : "input");
				snprintf(buffer, 1024, "%c%d", var, j);
				strsubst(decl, "{name}", buffer);
				fprintf(f, "  %s;\n", decl.c_str());
			}
			if (var == 'b')
				var = 'x';
			fprintf(f, "\n");
		}

		int total_y_size = 0;
		for (int j = 0; j < SIZE(arg_types)*3; j++)
			total_y_size += atoi(arg_types[j % SIZE(arg_types)][2]);
		fprintf(f, "  output [%d:0] y;\n", total_y_size-1);

		fprintf(f, "  assign y = {");
		for (int j = 0; j < SIZE(arg_types)*3; j++)
			fprintf(f, "%sy%d", j ? "," : "", j);
		fprintf(f, "};\n");
		fprintf(f, "\n");

		for (int j = 0; j < SIZE(arg_types)*3; j++) {
			std::string decl = arg_types[j % SIZE(arg_types)][0];
			strsubst(decl, "{dir}", "localparam");
			snprintf(buffer, 1024, "p%d", j);
			strsubst(decl, "{name}", buffer);
			fprintf(f, "  %s = ", decl.c_str());
			print_expression(f, 1 + xorshift32() % 15, 0, false, false, true);
			fprintf(f, ";\n");
		}
		fprintf(f, "\n");

		for (int j = 0; j < SIZE(arg_types)*3; j++) {
			fprintf(f, "  assign y%d = ", j);
			print_expression(f, 1 + xorshift32() % 20, 0, false, false, false);
			fprintf(f, ";\n");
		}

		fprintf(f, "endmodule\n");
		fclose(f);
	}
#endif

	return 0;
}

