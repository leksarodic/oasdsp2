//////////////////////////////////////////////////////////////////////////////
// *
// * Predmetni projekat iz predmeta OAiS DSP 2
// * Godina: 2018
// *
// * Zadatak: Ekvalizacija audio signala
// * Autor: Aleksa Rodic
// *                                                                          
// *                                                                          
/////////////////////////////////////////////////////////////////////////////

#include "stdio.h"
#include "ezdsp5535.h"
#include "ezdsp5535_i2c.h"
#include "aic3204.h"
#include "ezdsp5535_aic3204_dma.h"
#include "ezdsp5535_i2s.h"
#include "ezdsp5535_sar.h"
#include "print_number.h"
#include "math.h"

#include "iir.h"
#include "processing.h"

/* Frekvencija odabiranja */
#define SAMPLE_RATE 8000L

#define PI 3.14159265

#define VREDNOST01 3277

/* Niz za smestanje ulaznih i izlaznih odbiraka */
#pragma DATA_ALIGN(sampleBufferL,4)
Int16 sampleBufferL[AUDIO_IO_SIZE];
#pragma DATA_ALIGN(sampleBufferR,4)
Int16 sampleBufferR[AUDIO_IO_SIZE];

/* Koeficijenti */
Int16 lowpass_koeficijenti[4];
Int16 highpass_koeficijenti[4];
Int16 peek1_koeficijenti[6];
Int16 peek2_koeficijenti[6];

/* Istorija IIR filtera */
Int16 istorija_x_lp[2], istorija_y_lp[2];
Int16 istorija_x_hp[2], istorija_y_hp[2];
Int16 istorija_x_p1[3], istorija_y_p1[3];
Int16 istorija_x_p2[3], istorija_y_p2[3];

void istroija_na_0() {
	int i;

	for (i = 0; i < 2; i++) {
		istorija_x_lp[i] = 0;
		istorija_y_lp[i] = 0;
		istorija_x_hp[i] = 0;
		istorija_y_hp[i] = 0;
	}

	for (i = 0; i < 3; i++) {
		istorija_x_p1[i] = 0;
		istorija_y_p1[i] = 0;
		istorija_x_p2[i] = 0;
		istorija_y_p2[i] = 0;
	}
}

/* Racunanje alpha i beta */
float racunanje_alpha(Int16 frekvencija) {
	// Granicna ucestalnost i sirina opsega imaju istu formulu

	float w0 = (2 * PI * frekvencija) / SAMPLE_RATE;

	return (1 - sin(w0)) / cos(w0);
}

float racunanje_beta(Int16 frekvencija) {
	// Centralna ucestalost

	float w0 = (2 * PI * frekvencija) / SAMPLE_RATE;

	return cos(w0);
}

Int16 zadate_frekvencije[6] = { 140, 390, 200, 2935, 1905, 5500 };

/* Izlazne vrednosti */
// Testing
Int16 shelving_lowpass[AUDIO_IO_SIZE];
Int16 shelving_highpass[AUDIO_IO_SIZE];
Int16 shelving_peek1[AUDIO_IO_SIZE];

// Zadatak 5
Int16 eq_output[AUDIO_IO_SIZE];

/* Tasteri */
Uint16 getKey() {
	// Jednom pritisnut taster je stvarno jednom pritisnut
	Uint16 prethodni = NoKey;
	Uint16 trenutni = EZDSP5535_SAR_getKey();

	if (trenutni == prethodni) {
		return NoKey;
	} else {
		prethodni = trenutni;
		return trenutni;
	}
}

Int16 odabrano_K = 0;  // odabrano_K dobija vrednost 0-3

//Int16 vrednosti_K[4] = { 32767, 32767, 32767, 32767 };
Int16 vrednosti_K[4] = { 8200, 24500, 3200, 13000 };

void main(void) {
	int i;

	/* Inicijalizaija razvojne ploce */
	EZDSP5535_init();

	/* Inicijalizacija kontrolera za ocitavanje vrednosti pritisnutog dugmeta*/
	EZDSP5535_SAR_init();

	/* Inicijalizacija LCD kontrolera */
	initPrintNumber();

	printf("\n Ekvalizacija audio signala \n");

	/* Inicijalizacija veze sa AIC3204 kodekom (AD/DA) */
	aic3204_hardware_init();

	aic3204_set_input_filename("../input1.pcm");
	aic3204_set_output_filename("../output1.pcm");

	/* Inicijalizacija AIC3204 kodeka */
	aic3204_init();

	aic3204_dma_init();

	/* Postavljanje vrednosti frekvencije odabiranja i pojacanja na kodeku */
	set_sampling_frequency_and_gain(SAMPLE_RATE, 0);

	// Postavljamo istoriju na nulu
	istroija_na_0();

	// Racunanje koeficijenata
	// Testing puropose only
	calculateShelvingCoeff(0.3, lowpass_koeficijenti);
	calculateShelvingCoeff(-0.3, highpass_koeficijenti);
	calculatePeekCoeff(0.7, 0, peek1_koeficijenti);

	// Zadatak 5
	calculateShelvingCoeff(racunanje_alpha(zadate_frekvencije[0]),
			lowpass_koeficijenti);
	calculatePeekCoeff(racunanje_alpha(zadate_frekvencije[2]),
			racunanje_beta(zadate_frekvencije[1]), peek1_koeficijenti);
	calculatePeekCoeff(racunanje_alpha(zadate_frekvencije[4]),
			racunanje_beta(zadate_frekvencije[3]), peek2_koeficijenti);
	calculateShelvingCoeff(racunanje_alpha(zadate_frekvencije[5]),
			highpass_koeficijenti);

	// Zadatak 6
	clearLCD();

	// Dirakov impuls, testing purpose only
	Int16 dirac_impuls[AUDIO_IO_SIZE];
	dirac_impuls[0] = 16000;
	for (i = 1; i < AUDIO_IO_SIZE; i++) {
		dirac_impuls[i] = 0;
	}

	while (1) {
		aic3204_read_block(sampleBufferL, sampleBufferR);

		Uint16 taster = getKey();
		switch (taster) {
		case SW1:
			odabrano_K = (odabrano_K + 1) % 4;
			printChar(odabrano_K + '0');  // ispis karaktera, ascii tablea
			break;
		case SW2:
			vrednosti_K[odabrano_K] =
					((vrednosti_K[odabrano_K] - VREDNOST01) < 0) ?
							32767 : (vrednosti_K[odabrano_K] - VREDNOST01); // smanjujemo po 0.1 (3277), ako je <0 vrati se na 32767
			break;
		}

		for (i = 0; i < AUDIO_IO_SIZE; i++) {
			// Testing purpose only
			/* Zadaci 1-4
			 shelving_lowpass[i] = shelvingLP(dirac_impuls[i], lowpass_koeficijenti, istorija_x_lp, istorija_y_lp, 8192);  // k = 0.25
			 //shelving_lowpass[i] = shelvingLP(dirac_impuls[i], lowpass_koeficijenti, istorija_x_lp, istorija_y_lp, 24576);  // k = 0.75

			 shelving_highpass[i] = shelvingHP(dirac_impuls[i], highpass_koeficijenti, istorija_x_hp, istorija_y_hp, 8192);  // k = 0.25
			 //shelving_highpass[i] = shelvingHP(dirac_impuls[i], highpass_koeficijenti, istorija_x_hp, istorija_y_hp, 24576);  // k = 0.75

			 shelving_peek1[i] = shelvingPeek(dirac_impuls[i], peek1_koeficijenti, istorija_x_p1, istorija_y_p1, 8192);  // k = 0.25
			 //shelving_peek1[i] = shelvingPeek(dirac_impuls[i], peek1_koeficijenti, istorija_x_p1, istorija_y_p1, 24576);  // k = 0.75
			 */

			/* Zadatak 5
			 eq_output[i] = shelvingLP(dirac_impuls[i], lowpass_koeficijenti, istorija_x_lp, istorija_y_lp, vrednosti_K[0]);
			 eq_output[i] = shelvingPeek(eq_output[i], peek1_koeficijenti, istorija_x_p1, istorija_y_p1, vrednosti_K[1]);
			 eq_output[i] = shelvingPeek(eq_output[i], peek2_koeficijenti, istorija_x_p2, istorija_y_p2, vrednosti_K[2]);
			 eq_output[i] = shelvingHP(eq_output[i], highpass_koeficijenti, istorija_x_hp, istorija_y_hp, vrednosti_K[3]);
			 */

			// Funkcionalnost za ulazni stream
			sampleBufferL[i] = shelvingLP(sampleBufferL[i],
					lowpass_koeficijenti, istorija_x_lp, istorija_y_lp,
					vrednosti_K[0]);
			sampleBufferL[i] = shelvingPeek(sampleBufferL[i],
					peek1_koeficijenti, istorija_x_p1, istorija_y_p1,
					vrednosti_K[1]);
			sampleBufferL[i] = shelvingPeek(sampleBufferL[i],
					peek2_koeficijenti, istorija_x_p2, istorija_y_p2,
					vrednosti_K[2]);
			sampleBufferL[i] = shelvingHP(sampleBufferL[i],
					highpass_koeficijenti, istorija_x_hp, istorija_y_hp,
					vrednosti_K[3]);

		}

		aic3204_write_block(sampleBufferL, sampleBufferR);
	}

	/* Prekid veze sa AIC3204 kodekom */
	aic3204_disable();

	printf("\n***Kraj programa***\n");
	SW_BREAKPOINT
	;
}

