#include "processing.h"
#include "iir.h"

#define INT16NEGATIVE 32768
#define INT16POSITIVE 32767

void calculateShelvingCoeff(float c_alpha, Int16* output) {
	if (c_alpha < 0) {
		output[0] = c_alpha * INT16NEGATIVE;
		output[3] = -c_alpha * INT16POSITIVE;
	} else {
		output[0] = c_alpha * INT16POSITIVE;
		output[3] = -c_alpha * INT16NEGATIVE;
	}

	output[1] = -INT16NEGATIVE;
	output[2] = INT16POSITIVE;
}

void calculatePeekCoeff(float c_alpha, float c_beta, Int16* output) {
	Int16 alpha_deljenje_sa_2 = c_alpha / 2;

	if (c_alpha < 0) {
		output[0] = alpha_deljenje_sa_2 * INT16NEGATIVE;
		output[5] = alpha_deljenje_sa_2 * INT16NEGATIVE;
	} else {
		output[0] = alpha_deljenje_sa_2 * INT16POSITIVE;
		output[5] = alpha_deljenje_sa_2 * INT16POSITIVE;
	}

	Int16 izraz = (-c_beta * (1 + c_alpha));
	izraz = izraz / 3; // delimo sa 3 jer imamo 3 vrednosti. -2 * (1 + 2) = -6 / 3 = -2 i onda u output cuvamo polovinu

	Int16 izraz_deljenje_sa_2 = izraz / 2;

	if (izraz < 0) {
		output[1] = izraz_deljenje_sa_2 * INT16NEGATIVE;
		output[4] = izraz_deljenje_sa_2 * INT16NEGATIVE;
	} else {
		output[1] = izraz_deljenje_sa_2 * INT16POSITIVE;
		output[4] = izraz_deljenje_sa_2 * INT16POSITIVE;
	}

	output[2] = INT16POSITIVE;
	output[3] = INT16POSITIVE;
}

Int16 shelvingHP(Int16 input, Int16* coeff, Int16* x_history, Int16* y_history,
		Int16 k) {
	Int16 suma1, suma2, izlazni_odbirak;
	Int16 primena_filtera;

	// na ulazni odbirak primenjujemo filter
	primena_filtera = first_order_IIR(input, coeff, x_history, y_history);
	primena_filtera = primena_filtera >> 1;

	// sumiramo ulazni odbirak i filtriran odbirak
	suma1 = (input >> 1) + primena_filtera;
	suma1 = suma1 >> 1;
	suma1 = _smpy(suma1, k);

	// sumiramo ulazni odbirak i negativan filtriran odbirak
	suma2 = (input >> 1) - primena_filtera;
	suma2 = suma2 >> 1;

	// kreiramo izlazni odbirak
	izlazni_odbirak = suma1 + suma2;

	return izlazni_odbirak;
}

Int16 shelvingLP(Int16 input, Int16* coeff, Int16* x_history, Int16* y_history,
		Int16 k) {
	Int16 suma1, suma2, izlazni_odbirak;
	Int16 primena_filtera;

	// na ulazni odbirak primenjujemo filter
	primena_filtera = first_order_IIR(input, coeff, x_history, y_history);
	primena_filtera = primena_filtera >> 1;

	// sumiramo ulazni odbirak i filtriran odbirak
	suma1 = (input >> 1) + primena_filtera;
	suma1 = suma1 >> 1;

	// sumiramo ulazni odbirak i negativan filtriran odbirak
	suma2 = (input >> 1) - primena_filtera;
	suma2 = suma2 >> 1;
	suma2 = _smpy(suma2, k);

	// kreiramo izlazni odbirak
	izlazni_odbirak = suma1 + suma2;

	return izlazni_odbirak;
}

Int16 shelvingPeek(Int16 input, Int16* coeff, Int16* x_history,
		Int16* y_history, Int16 k) {
	Int16 suma1, suma2, izlazni_odbirak;
	Int16 primena_filtera;

	// na ulazni odbirak primenjujemo filter
	primena_filtera = second_order_IIR(input, coeff, x_history, y_history);
	primena_filtera = primena_filtera >> 1;

	// sumiramo ulazni odbirak i filtriran odbirak
	suma1 = (input >> 1) + primena_filtera;
	suma1 = suma1 >> 1;

	// sumiramo ulazni odbirak i negativan filtriran odbirak
	suma2 = (input >> 1) - primena_filtera;
	suma2 = suma2 >> 1;
	suma2 = _smpy(suma2, k);

	// kreiramo izlazni odbirak
	izlazni_odbirak = suma1 + suma2;

	return izlazni_odbirak;
}
