/******************************************************************/
/******************************************************************/
/***							      	***/
/***           	MinMax / Jeu d'échecs / ESI 2017            	***/
/***							      	***/
/******************************************************************/
/******************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>  	// pour INT_MAX


#define MAX +1		// Niveau Maximisant
#define MIN -1  	// Niveau Minimisant

#define INFINI INT_MAX


// Type d'une configuration formant l'espace de recherche
struct config {
	char mat[8][8];			// Echiquier
	int val;			// Estimation de la config
	char xrN, yrN, xrB, yrB;	// Positions des rois Noir et Blanc
	char roqueN, roqueB;		// Indicateurs de roque pour N et B : 
					// 'g' grand roque non réalisable  
					// 'p' petit roque non réalisable 
					// 'n' non réalisable des 2 cotés
					// 'r' réalisable (valeur initiale)
					// 'e' effectué
};


int num_coup = 0;  	// compteur de coups effectués


// vecteurs pour générer les différents déplacements par type de pièce ...
// cavalier :
int dC[8][2] = { {-2,+1} , {-1,+2} , {+1,+2} , {+2,+1} , {+2,-1} , {+1,-2} , {-1,-2} , {-2,-1} };
// fou (indices impairs), tour (indices pairs), reine et roi (indices pairs et impairs):
int D[8][2] = { {+1,0} , {+1,+1} , {0,+1} , {-1,+1} , {-1,0} , {-1,-1} , {0,-1} , {+1,-1} }; 


// evalue avec alpha beta la configuration 'conf' du joueur 'mode' en descendant de 'niv' niveaux
int minmax_ab( struct config *conf, int mode, int niv, int min, int max, int auprofit, int k );

// fonction qui teste si une position donnée est menacée par un joueur 'mode'
int caseMenaceePar( int mode, int x, int y, struct config *conf );

/* Copie la configuration c1 dans c2  */
void copier( struct config *c1, struct config *c2 ) 
{
	int i, j;

	for (i=0; i<8; i++)
		for (j=0; j<8; j++)
			c2->mat[i][j] = c1->mat[i][j];

	c2->val = c1->val;
	c2->xrB = c1->xrB;
	c2->yrB = c1->yrB;
	c2->xrN = c1->xrN;
	c2->yrN = c1->yrN;

	c2->roqueB = c1->roqueB;
	c2->roqueN = c1->roqueN;
} // copier


/* Teste si les conf c1 et c2 sont égales */
int egal(char c1[8][8], char c2[8][8] )
{
	int i, j;

	for (i=0; i<8; i++)
		for (j=0; j<8; j++)
			if (c1[i][j] != c2[i][j]) return 0;
	return 1;
} // egal




/***********************************************************/
/*********** Partie:  Evaluations et Estimations ***********/
/***********************************************************/


/* Teste s'il n'y a aucun coup possible dans la configuration conf */
int AucunCoupPossible( struct config *conf )
{
      	// ... A completer pour les matchs nuls
	// ... vérifier que generer_succ retourne 0 configurations filles ...
	return 0;

} // AucunCoupPossible


/* Teste si conf représente une fin de partie et retourne dans 'cout' la valeur associée */
int feuille( struct config *conf, int *cout )
{
	//int i, j, rbx, rnx, rby, rny;
	
	*cout = 0;

	// Si victoire pour les Noirs cout = -100
	if ( conf->xrB == -1 ) { 
	   *cout = -100;
	   return 1; 
	}

	// Si victoire pour les Blancs cout = +100
	if ( conf->xrN == -1 ) {
	   *cout = +100;
	   return 1;
	}

	// Si Match nul cout = 0
	if (  conf->xrB != -1 &&  conf->xrN != -1 && AucunCoupPossible( conf ) )
	   return 1;

	// Sinon ce n'est pas une config feuille 
	return 0;

}  // feuille


/* Retourne une estimation de la configuration conf */

int estim1( struct config *conf )
{

	int i, j, ScrQte;
	int pionB = 0, pionN = 0, cfB = 0, cfN = 0, tB = 0, tN = 0, nB = 0, nN = 0;
	

	// parties : nombre de pièces 
	for (i=0; i<8; i++)
	   for (j=0; j<8; j++) {
		switch (conf->mat[i][j]) {
		   case 'p' : pionB++;   break;
		   case 'c' : 
		   case 'f' : cfB++;  break;
		   case 't' : tB++; break;
		   case 'n' : nB++;  break;

		   case -'p' : pionN++;  break;
		   case -'c' : 
		   case -'f' : cfN++;  break;
		   case -'t' : tN++; break;
		   case -'n' : nN++;  break;
		}
	   }

	// Somme pondérée de pièces de chaque joueur. 
	// Le facteur 1.4 pour ne pas sortir de l'intervalle ]-100 , +100[
	ScrQte = ( (pionB*2 + cfB*6 + tB*8 + nB*20) - (pionN*2 + cfN*6 + tN*8 + nN*20) ) * 100.0/76;

	if (ScrQte > 95) ScrQte = 95;		// pour rétrécir l'intervalle à
	if (ScrQte < -95) ScrQte = -95;		// ]-95 , +95[ car ce n'est qu'une estimation

	return ScrQte;

} // estim1


// estimation basée sur le nb de pieces, l'occupation, la défense du roi et les roques
int estim2(  struct config *conf )
{
	int i, j, a, b, stop, bns, ScrQte, ScrDisp, ScrDfs, ScrDivers, Score;
	int pionB = 0, pionN = 0, cfB = 0, cfN = 0, tB = 0, tN = 0, nB = 0, nN = 0;
	int occCentreB = 0, occCentreN = 0, protectRB = 0, protectRN = 0, divB = 0, divN = 0;

	// parties : nombre de pièces et occupation du centre
	for (i=0; i<8; i++)
	   for (j=0; j<8; j++) {
		bns = 0;  // bonus pour l'occupation du centre de l'échiquier
		if (i>1 && i<6 && j>1 && j<6 ) bns = 1; 
		if (i>2 && i<5 && j>2 && j<5 ) bns = 2; 
		switch (conf->mat[i][j]) {
		   case 'p' : pionB++; occCentreB += bns;  break;
		   case 'c' : 
		   case 'f' : cfB++; occCentreB += 4*bns; break;
		   case 't' : tB++; break;
		   case 'n' : nB++; occCentreB += 1*bns; break;

		   case -'p' : pionN++; occCentreN += bns; break;
		   case -'c' : 
		   case -'f' : cfN++; occCentreN += 4*bns; break;
		   case -'t' : tN++; break;
		   case -'n' : nN++; occCentreN += 1*bns; break;
		}
	   }

	ScrQte = ( (pionB + cfB*6 + tB*8 + nB*20) - (pionN + cfN*6 + tN*8 + nN*20) );

	ScrDisp = occCentreB - occCentreN;
	
	// partie : défense du roi B ...
	for (i=0; i<8; i += 1) {
	   // traitement des 8 directions paires et impaires
	   stop = 0;
	   a = conf->xrB + D[i][0];
	   b = conf->yrB + D[i][1];	  	 
	   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
		if ( conf->mat[a][b] != 0 )  stop = 1;
		else {
		    a = a + D[i][0];
		    b = b + D[i][1];
		}
	   if ( stop ) 
		if ( conf->mat[a][b] > 0 ) protectRB++; 
	} // for

	// partie : défense du roi N ...
	for (i=0; i<8; i += 1) {
	   // traitement des 8 directions paires et impaires
	   stop = 0;
	   a = conf->xrN + D[i][0];
	   b = conf->yrN + D[i][1];	  	 
	   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
		if ( conf->mat[a][b] != 0 )  stop = 1;
		else {
		    a = a + D[i][0];
		    b = b + D[i][1];
		}
	   if ( stop ) 
		if ( conf->mat[a][b] < 0 ) protectRN++; 
	} // for	
 	
	ScrDfs = protectRB - protectRN;

	// Partie : autres considérations ...
	if ( conf->roqueB == 'e' ) divB = 8;	// favoriser les roques B
	if ( conf->roqueB == 'r' ) divB = 4;
	if ( conf->roqueB == 'p' || conf->roqueB == 'g' ) divB = 2;

	if ( conf->roqueN == 'e' ) divN = 8;	// favoriser les roques N
	if ( conf->roqueN == 'r' ) divN = 4;
	if ( conf->roqueN == 'p' || conf->roqueN == 'g' ) divN = 2;

	ScrDivers = divB - divN;

	Score = (ScrQte + ScrDisp + ScrDfs + ScrDivers) * 100.0/(68+32+8+8);

        if (Score > 98 ) Score = 98;
        if (Score < -98 ) Score = -98;

	return Score;

} // estim2


/* prise en compte uniquement des pièces maitresses et défense du roi */
int estim3( struct config *conf )
{

	int i, j, a, b, stop, ScrQte, ScrDfs, Score;
	int cfB = 0, cfN = 0, tB = 0, tN = 0, nB = 0, nN = 0;
	int protectRB = 0, protectRN = 0;

	// parties : nombre de pièces maitresses 
	for (i=0; i<8; i++)
	   for (j=0; j<8; j++) {
		switch (conf->mat[i][j]) {
		   case 'c' : 
		   case 'f' : cfB++;  break;
		   case 't' : tB++; break;
		   case 'n' : nB++;  break;

		   case -'c' : 
		   case -'f' : cfN++;  break;
		   case -'t' : tN++; break;
		   case -'n' : nN++;  break;
		}
	   }

	// Somme pondérée de pièces de chaque joueur. 
	ScrQte = ( (cfB + tB*2 + nB*4) - (cfN + tN*2 + nN*4) );


	// partie : défense du roi B ...
	for (i=0; i<8; i += 1) {
	   // traitement des 8 directions paires et impaires
	   stop = 0;
	   a = conf->xrB + D[i][0];
	   b = conf->yrB + D[i][1];	  	 
	   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
		if ( conf->mat[a][b] != 0 )  stop = 1;
		else {
		    a = a + D[i][0];
		    b = b + D[i][1];
		}
	   if ( stop ) 
		if ( conf->mat[a][b] > 0 ) protectRB++;
		else protectRB--; 
	} // for

	// partie : défense du roi N ...
	for (i=0; i<8; i += 1) {
	   // traitement des 8 directions paires et impaires
	   stop = 0;
	   a = conf->xrN + D[i][0];
	   b = conf->yrN + D[i][1];	  	 
	   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
		if ( conf->mat[a][b] != 0 )  stop = 1;
		else {
		    a = a + D[i][0];
		    b = b + D[i][1];
		}
	   if ( stop ) 
		if ( conf->mat[a][b] < 0 ) protectRN++;
		else protectRN--;  
	} // for	


	ScrDfs = protectRB - protectRN;

	Score = (ScrQte + ScrDfs) * 100.0 / (12 + 16);

        if (Score > 98 ) Score = 98;
        if (Score < -98 ) Score = -98;

	return Score;

} // estim3


// estimation basée sur le nb de pieces et les menaces
int estim4( struct config *conf )
{

	int i, j, Score;
	int pionB = 0, pionN = 0, cfB = 0, cfN = 0, tB = 0, tN = 0, nB = 0, nN = 0;
	int npmB = 0, npmN = 0;

	// parties : nombre de pièces 
	for (i=0; i<8; i++)
	   for (j=0; j<8; j++) {
		switch (conf->mat[i][j]) {
		   case 'p' : pionB++;   break;
		   case 'c' : 
		   case 'f' : cfB++;  break;
		   case 't' : tB++; break;
		   case 'n' : nB++;  break;

		   case -'p' : pionN++;  break;
		   case -'c' : 
		   case -'f' : cfN++;  break;
		   case -'t' : tN++; break;
		   case -'n' : nN++;  break;
		}
	   }

	for (i=0; i<8; i++)
	   for (j=0; j<8; j++) {
		if ( conf->mat[i][j] < 0 && caseMenaceePar(MAX, i, j, conf) ) {
		   npmB++;
		   if ( conf->mat[i][j] == -'c' || conf->mat[i][j] == -'f' )
		   	npmB++;
		   if ( conf->mat[i][j] == -'t' || conf->mat[i][j] == -'n' )
		   	npmB += 2;
		   if ( conf->mat[i][j] == -'r' )
		   	npmB += 4;
		}
		if ( conf->mat[i][j] > 0 && caseMenaceePar(MIN, i, j, conf) ) {
		   npmN++;
		   if ( conf->mat[i][j] == 'c' || conf->mat[i][j] == 'f' )
		   	npmN++;
		   if ( conf->mat[i][j] == 't' || conf->mat[i][j] == 'n' )
		   	npmN += 2;
		   if ( conf->mat[i][j] == 'r' )
		   	npmN += 4;
		}
	   }


	Score = ( 2*((pionB*2 + cfB*6 + tB*8 + nB*20) - (pionN*2 + cfN*6 + tN*8 + nN*20)) + \
		  (npmB - npmN) ) * 100.0/(2*76+16+21);

	if (Score > 95) Score = 95;		// pour rétrécir l'intervalle à
	if (Score < -95) Score = -95;		// ]-95 , +95[ car ce n'est qu'une estimation

	return Score;

} // estim4


// estimation basée sur le nb de pieces et l'occupation
int estim5(  struct config *conf )
{
	int i, j, a, b, stop, bns, ScrQte, ScrDisp, ScrDfs, ScrDivers, Score;
	int pionB = 0, pionN = 0, cfB = 0, cfN = 0, tB = 0, tN = 0, nB = 0, nN = 0;
	int occCentreB = 0, occCentreN = 0, protectRB = 0, protectRN = 0, divB = 0, divN = 0;

	// parties : nombre de pièces et occupation du centre
	for (i=0; i<8; i++)
	   for (j=0; j<8; j++) {
		bns = 0;  // bonus pour l'occupation du centre de l'échiquier
		if (i>1 && i<6 && j>1 && j<6 ) bns = 1; 
		if (i>2 && i<5 && j>2 && j<5 ) bns = 2; 
		switch (conf->mat[i][j]) {
		   case 'p' : pionB++; occCentreB += bns;  break;
		   case 'c' : 
		   case 'f' : cfB++; occCentreB += 4*bns; break;
		   case 't' : tB++; break;
		   case 'n' : nB++; occCentreB += 1*bns; break;

		   case -'p' : pionN++; occCentreN += bns; break;
		   case -'c' : 
		   case -'f' : cfN++; occCentreN += 4*bns; break;
		   case -'t' : tN++; break;
		   case -'n' : nN++; occCentreN += 1*bns; break;
		}
	   }

	ScrQte = ( (pionB + cfB*6 + tB*8 + nB*20) - (pionN + cfN*6 + tN*8 + nN*20) );

	ScrDisp = occCentreB - occCentreN;

	Score = (2*ScrQte + ScrDisp) * 100.0/(2*68+32);

        if (Score > 98 ) Score = 98;
        if (Score < -98 ) Score = -98;

	return Score;

} // estim5


int estim( struct config *conf, int auprofit )
{
   	if ( auprofit == MAX )
		return estim1(conf);
	else
	    if (num_coup < 30 )
		return estim5(conf);
	    else
		return estim4(conf);
}



/***********************************************************/
/*********** Partie:  Génération des Successeurs ***********/
/***********************************************************/


/* Génère dans T les configurations obtenues à partir de conf lorsqu'un pion atteint la limite de l'échiq */
void transformPion( struct config *conf, int a, int b, int x, int y, struct config T[], int *n )
{
	int signe = +1;
	if (conf->mat[a][b] < 0 ) signe = -1;
	copier(conf, &T[*n]);
	T[*n].mat[a][b] = 0;
	T[*n].mat[x][y] = signe *'n';
	(*n)++;
	copier(conf, &T[*n]);
	T[*n].mat[a][b] = 0;
	T[*n].mat[x][y] = signe *'c';
	(*n)++;
	copier(conf, &T[*n]);
	T[*n].mat[a][b] = 0;
	T[*n].mat[x][y] = signe *'f';
	(*n)++;
	copier(conf, &T[*n]);
	T[*n].mat[a][b] = 0;
	T[*n].mat[x][y] = signe *'t';
	(*n)++;

} // transformPion


// Vérifie si la case (x,y) est menacée par une des pièces du joueur 'mode'
int caseMenaceePar( int mode, int x, int y, struct config *conf )
{
	int i, j, a, b, stop;

	// menace par le roi ...
	for (i=0; i<8; i += 1) {
	   // traitement des 8 directions paires et impaires
	   a = x + D[i][0];
	   b = y + D[i][1];	  	 
	   if ( a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
		if ( conf->mat[a][b]*mode == 'r' ) return 1;
	} // for

	// menace par cavalier ...
	for (i=0; i<8; i++)
	   if ( x+dC[i][0] <= 7 && x+dC[i][0] >= 0 && y+dC[i][1] <= 7 && y+dC[i][1] >= 0 )
		if ( conf->mat[ x+dC[i][0] ] [ y+dC[i][1] ] * mode == 'c' )  
		   return 1;

	// menace par pion ...
	if ( (x-mode) >= 0 && (x-mode) <= 7 && y > 0 && conf->mat[x-mode][y-1]*mode == 'p' )
	   return 1;
	if ( (x-mode) >= 0 && (x-mode) <= 7 && y < 7 && conf->mat[x-mode][y+1]*mode == 'p' )
	   return 1;

	// menace par fou, tour ou reine ...
	for (i=0; i<8; i += 1) {
	   // traitement des 8 directions paires et impaires
	   stop = 0;
	   a = x + D[i][0];
	   b = y + D[i][1];	  	 
	   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
		if ( conf->mat[a][b] != 0 )  stop = 1;
		else {
		    a = a + D[i][0];
		    b = b + D[i][1];
		}
	   if ( stop )  {
		if ( conf->mat[a][b]*mode == 'f' && i % 2 != 0 ) return 1; 
		if ( conf->mat[a][b]*mode == 't' && i % 2 == 0 ) return 1;
		if ( conf->mat[a][b]*mode == 'n' ) return 1;
	   }
	} // for

	// sinon, aucune menace ...
	return 0;

} // caseMenaceePar


/* Génere dans T tous les coups possibles de la pièce (de couleur N) se trouvant à la pos x,y */
void deplacementsN(struct config *conf, int x, int y, struct config T[], int *n )
{
	int i, j, a, b, stop;

	switch(conf->mat[x][y]) {
	// mvmt PION ...
	case -'p' : 
		//***printf("PION N à la pos (%d,%d) \n", x,y);
		if ( x > 0 && conf->mat[x-1][y] == 0 ) {				// avance d'une case
			copier(conf, &T[*n]);
			T[*n].mat[x][y] = 0;
			T[*n].mat[x-1][y] = -'p';
			(*n)++;
			if ( x == 1 ) transformPion( conf, x, y, x-1, y, T, n );
		}
		if ( x == 6 && conf->mat[5][y] == 0 && conf->mat[4][y] == 0) {	// avance de 2 cases
			copier(conf, &T[*n]);
			T[*n].mat[6][y] = 0;
			T[*n].mat[4][y] = -'p';
			(*n)++;
		}
		if ( x > 0 && y >0 && conf->mat[x-1][y-1] > 0 ) {		// mange à droite (en descendant)
			copier(conf, &T[*n]);
			T[*n].mat[x][y] = 0;
			T[*n].mat[x-1][y-1] = -'p';
			// cas où le roi adverse est pris...
			if (T[*n].xrB == x-1 && T[*n].yrB == y-1) { 
				T[*n].xrB = -1; T[*n].yrB = -1; 
			}

			(*n)++;
			if ( x == 1 ) transformPion( conf, x, y, x-1, y-1, T, n ); 
		}
		if ( x > 0 && y < 7 && conf->mat[x-1][y+1] > 0 ) {		// mange à gauche (en descendant)
			copier(conf, &T[*n]);
			T[*n].mat[x][y] = 0;
			T[*n].mat[x-1][y+1] = -'p';
			// cas où le roi adverse est pris...
			if (T[*n].xrB == x-1 && T[*n].yrB == y+1) { 
				T[*n].xrB = -1; T[*n].yrB = -1; 
			}

			(*n)++;
			if ( x == 1 ) transformPion( conf, x, y, x-1, y+1, T, n );
		}
		break;

	// mvmt CAVALIER ...
	case -'c' : 
		for (i=0; i<8; i++)
		   if ( x+dC[i][0] <= 7 && x+dC[i][0] >= 0 && y+dC[i][1] <= 7 && y+dC[i][1] >= 0 )
			if ( conf->mat[ x+dC[i][0] ] [ y+dC[i][1] ] >= 0 )  {
			   copier(conf, &T[*n]);
			   T[*n].mat[x][y] = 0;
			   T[*n].mat[ x+dC[i][0] ][ y+dC[i][1] ] = -'c';
			   // cas où le roi adverse est pris...
			   if (T[*n].xrB == x+dC[i][0] && T[*n].yrB == y+dC[i][1]) { 
				T[*n].xrB = -1; T[*n].yrB = -1; 
			   }

			   (*n)++;
			}
		break;

	// mvmt FOU ...
	case -'f' : 
		for (i=1; i<8; i += 2) {
		   // traitement des directions impaires (1, 3, 5 et 7)
		   stop = 0;
		   a = x + D[i][0];
		   b = y + D[i][1];	  	 
		   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
			if ( conf->mat[ a ] [ b ] < 0 )  stop = 1;
			else {
			   copier(conf, &T[*n]);
			   T[*n].mat[x][y] = 0;
			   if ( T[*n].mat[a][b] > 0 ) stop = 1;
			   T[*n].mat[a][b] = -'f';
			   // cas où le roi adverse est pris...
			   if (T[*n].xrB == a && T[*n].yrB == b) { T[*n].xrB = -1; T[*n].yrB = -1; }

			   (*n)++;
		   	   a = a + D[i][0];
		   	   b = b + D[i][1];
			}
		   } // while
		} // for
		break;

	// mvmt TOUR ...
	case -'t' : 
		for (i=0; i<8; i += 2) {
		   // traitement des directions paires (0, 2, 4 et 6)
		   stop = 0;
		   a = x + D[i][0];
		   b = y + D[i][1];	  	 
		   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
			if ( conf->mat[ a ] [ b ] < 0 )  stop = 1;
			else {
			   copier(conf, &T[*n]);
			   T[*n].mat[x][y] = 0;
			   if ( T[*n].mat[a][b] > 0 ) stop = 1;
			   T[*n].mat[a][b] = -'t';
			   // cas où le roi adverse est pris...
			   if (T[*n].xrB == a && T[*n].yrB == b) { T[*n].xrB = -1; T[*n].yrB = -1; }

			   if ( conf->roqueN != 'e' && conf->roqueN != 'n' ) {
			      if ( x == 7 && y == 0 && conf->roqueN != 'p')
			   	T[*n].roqueN = 'g'; // le grand roque ne sera plus possible
			      else if ( x == 7 && y == 0 )
			   	   T[*n].roqueN = 'n'; // ni le grand roque ni le petit roque ne seront possibles
			      if ( x == 7 && y == 7 && conf->roqueN != 'g' )
			   	T[*n].roqueN = 'p'; // le petit roque ne sera plus possible
			      else if ( x == 7 && y == 7 )
			   	   T[*n].roqueN = 'n'; // ni le grand roque ni le petit roque ne seront possibles
			   }

			   (*n)++;
		   	   a = a + D[i][0];
		   	   b = b + D[i][1];
			}
		   } // while
		} // for
		break;

	// mvmt REINE ...
	case -'n' : 
		for (i=0; i<8; i += 1) {
		   // traitement des 8 directions paires et impaires
		   stop = 0;
		   a = x + D[i][0];
		   b = y + D[i][1];	  	 
		   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
			if ( conf->mat[ a ] [ b ] < 0 )  stop = 1;
			else {
			   copier(conf, &T[*n]);
			   T[*n].mat[x][y] = 0;
			   if ( T[*n].mat[a][b] > 0 ) stop = 1;
			   T[*n].mat[a][b] = -'n';
			   // cas où le roi adverse est pris...
			   if (T[*n].xrB == a && T[*n].yrB == b) { T[*n].xrB = -1; T[*n].yrB = -1; }

			   (*n)++;
		   	   a = a + D[i][0];
		   	   b = b + D[i][1];
			}
		   } // while
		} // for
		break;

	// mvmt ROI ...
	case -'r' : 
		// vérifier possibilité de faire un roque ...
		if ( conf->roqueN != 'n' && conf->roqueN != 'e' ) {
		   if ( conf->roqueN != 'g' && conf->mat[7][1] == 0 && conf->mat[7][2] == 0 && conf->mat[7][3] == 0 )
		      if ( !caseMenaceePar( MAX, 7, 1, conf ) && !caseMenaceePar( MAX, 7, 2, conf ) && \
			   !caseMenaceePar( MAX, 7, 3, conf ) && !caseMenaceePar( MAX, 7, 4, conf ) )  {
			// Faire un grand roque ...
			copier(conf, &T[*n]);
			T[*n].mat[7][4] = 0;
			T[*n].mat[7][0] = 0;
			T[*n].mat[7][2] = -'r'; T[*n].xrN = 7; T[*n].yrN = 2;
			T[*n].mat[7][3] = -'t';
			T[*n].roqueN = 'e'; // aucun roque ne sera plus possible à partir de cette config
			(*n)++;
		      }
		   if ( conf->roqueN != 'p' && conf->mat[7][5] == 0 && conf->mat[7][6] == 0 )
		      if ( !caseMenaceePar( MAX, 7, 4, conf ) && !caseMenaceePar( MAX, 7, 5, conf ) && \
			   !caseMenaceePar( MAX, 7, 6, conf ) )  {
			// Faire un petit roque ...
			copier(conf, &T[*n]);
			T[*n].mat[7][4] = 0;
			T[*n].mat[7][7] = 0;
			T[*n].mat[7][6] = -'r'; T[*n].xrN = 7; T[*n].yrN = 6;
			T[*n].mat[7][5] = -'t';
			T[*n].roqueN = 'e'; // aucun roque ne sera plus possible à partir de cette config
			(*n)++;

		      }
		}
			
		// vérifier les autres mouvements du roi ...
		for (i=0; i<8; i += 1) {
		   // traitement des 8 directions paires et impaires
		   a = x + D[i][0];
		   b = y + D[i][1];	  	 
		   if ( a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
			if ( conf->mat[a][b] >= 0 ) {
			   copier(conf, &T[*n]);
			   T[*n].mat[x][y] = 0;
			   T[*n].mat[a][b] = -'r'; T[*n].xrN = a; T[*n].yrN = b;
			   // cas où le roi adverse est pris...
			   if (T[*n].xrB == a && T[*n].yrB == b) { T[*n].xrB = -1; T[*n].yrB = -1; }

			   T[*n].roqueN = 'n'; // aucun roque ne sera plus possible à partir de cette config
			   (*n)++;
			}
		} // for
		break;

	}

} // deplacementsN


/* Génere dans T tous les coups possibles de la pièce (de couleur B) se trouvant à la pos x,y */
void deplacementsB(struct config *conf, int x, int y, struct config T[], int *n )
{
	int i, j, a, b, stop;

	switch(conf->mat[x][y]) {
	// mvmt PION ...
	case 'p' :  
		if ( x <7 && conf->mat[x+1][y] == 0 ) {				// avance d'une case
			copier(conf, &T[*n]);
			T[*n].mat[x][y] = 0;
			T[*n].mat[x+1][y] = 'p';
			(*n)++;
			if ( x == 6 ) transformPion( conf, x, y, x+1, y, T, n );
		}
		if ( x == 1 && conf->mat[2][y] == 0 && conf->mat[3][y] == 0) {	// avance de 2 cases
			copier(conf, &T[*n]);
			T[*n].mat[1][y] = 0;
			T[*n].mat[3][y] = 'p';
			(*n)++;
		}
		if ( x < 7 && y > 0 && conf->mat[x+1][y-1] < 0 ) {		// mange à gauche (en montant)
			copier(conf, &T[*n]);
			T[*n].mat[x][y] = 0;
			T[*n].mat[x+1][y-1] = 'p';
			// cas où le roi adverse est pris...
			if (T[*n].xrN == x+1 && T[*n].yrN == y-1) { 
				T[*n].xrN = -1; T[*n].yrN = -1; 
			}

			(*n)++;
			if ( x == 6 ) transformPion( conf, x, y, x+1, y-1, T, n );
		}
		if ( x < 7 && y < 7 && conf->mat[x+1][y+1] < 0 ) {		// mange à droite (en montant)
			copier(conf, &T[*n]);
			T[*n].mat[x][y] = 0;
			T[*n].mat[x+1][y+1] = 'p';
			// cas où le roi adverse est pris...
			if (T[*n].xrN == x+1 && T[*n].yrN == y+1) { 
				T[*n].xrN = -1; T[*n].yrN = -1; 
			}

			(*n)++;
			if ( x == 6 ) transformPion( conf, x, y, x+1, y+1, T, n );
		}
		break;

	// mvmt CAVALIER ...
	case 'c' : 
		for (i=0; i<8; i++)
		   if ( x+dC[i][0] <= 7 && x+dC[i][0] >= 0 && y+dC[i][1] <= 7 && y+dC[i][1] >= 0 )
			if ( conf->mat[ x+dC[i][0] ] [ y+dC[i][1] ] <= 0 )  {
			   copier(conf, &T[*n]);
			   T[*n].mat[x][y] = 0;
			   T[*n].mat[ x+dC[i][0] ][ y+dC[i][1] ] = 'c';
			   // cas où le roi adverse est pris...
			   if (T[*n].xrN == x+dC[i][0] && T[*n].yrN == y+dC[i][1]) { 
				T[*n].xrN = -1; T[*n].yrN = -1; 
			   }

			   (*n)++;
			}
		break;

	// mvmt FOU ...
	case 'f' : 
		for (i=1; i<8; i += 2) {
		   // traitement des directions impaires (1, 3, 5 et 7)
		   stop = 0;
		   a = x + D[i][0];
		   b = y + D[i][1];	  	 
		   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
			if ( conf->mat[ a ] [ b ] > 0 )  stop = 1;
			else {
			   copier(conf, &T[*n]);
			   T[*n].mat[x][y] = 0;
			   if ( T[*n].mat[a][b] < 0 ) stop = 1;
			   T[*n].mat[a][b] = 'f';
			   // cas où le roi adverse est pris...
			   if (T[*n].xrN == a && T[*n].yrN == b) { T[*n].xrN = -1; T[*n].yrN = -1; }

			   (*n)++;
		   	   a = a + D[i][0];
		   	   b = b + D[i][1];
			}
		   } // while
		} // for
		break;

	// mvmt TOUR ...
	case 't' : 
		for (i=0; i<8; i += 2) {
		   // traitement des directions paires (0, 2, 4 et 6)
		   stop = 0;
		   a = x + D[i][0];
		   b = y + D[i][1];	  	 
		   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
			if ( conf->mat[ a ] [ b ] > 0 )  stop = 1;
			else {
			   copier(conf, &T[*n]);
			   T[*n].mat[x][y] = 0;
			   if ( T[*n].mat[a][b] < 0 ) stop = 1;
			   T[*n].mat[a][b] = 't';
			   // cas où le roi adverse est pris...
			   if (T[*n].xrN == a && T[*n].yrN == b) { T[*n].xrN = -1; T[*n].yrN = -1; }

			   if ( conf->roqueB != 'e' && conf->roqueB != 'n' ) {
			     if ( x == 0 && y == 0 && conf->roqueB != 'p')
			   	T[*n].roqueB = 'g'; // le grand roque ne sera plus possible
			     else if ( x == 0 && y == 0 )
			   	   T[*n].roqueB = 'n'; // ni le grand roque ni le petit roque ne seront possibles
			     if ( x == 0 && y == 7 && conf->roqueB != 'g' )
			   	T[*n].roqueB = 'p'; // le petit roque ne sera plus possible
			     else if ( x == 0 && y == 7 )
			   	   T[*n].roqueB = 'n'; // ni le grand roque ni le petit roque ne seront possibles
			   }

			   (*n)++;
		   	   a = a + D[i][0];
		   	   b = b + D[i][1];
			}
		   } // while
		} // for
		break;

	// mvmt REINE ...
	case 'n' : 
		for (i=0; i<8; i += 1) {
		   // traitement des 8 directions paires et impaires
		   stop = 0;
		   a = x + D[i][0];
		   b = y + D[i][1];	  	 
		   while ( !stop && a >= 0 && a <= 7 && b >= 0 && b <= 7 ) {
			if ( conf->mat[ a ] [ b ] > 0 )  stop = 1;
			else {
			   copier(conf, &T[*n]);
			   T[*n].mat[x][y] = 0;
			   if ( T[*n].mat[a][b] < 0 ) stop = 1;
			   T[*n].mat[a][b] = 'n';
			   // cas où le roi adverse est pris...
			   if (T[*n].xrN == a && T[*n].yrN == b) { T[*n].xrN = -1; T[*n].yrN = -1; }

			   (*n)++;
		   	   a = a + D[i][0];
		   	   b = b + D[i][1];
			}
		   } // while
		} // for
		break;

	// mvmt ROI ...
	case 'r' : 
		// vérifier possibilité de faire un roque ...
		if ( conf->roqueB != 'n' && conf->roqueB != 'e' ) {
		   if ( conf->roqueB != 'g' && conf->mat[0][1] == 0 && conf->mat[0][2] == 0 && conf->mat[0][3] == 0 )
		      if ( !caseMenaceePar( MIN, 0, 1, conf ) && !caseMenaceePar( MIN, 0, 2, conf ) && \
			   !caseMenaceePar( MIN, 0, 3, conf ) && !caseMenaceePar( MIN, 0, 4, conf ) )  {
			// Faire un grand roque ...
			copier(conf, &T[*n]);
			T[*n].mat[0][4] = 0;
			T[*n].mat[0][0] = 0;
			T[*n].mat[0][2] = 'r'; T[*n].xrB = 0; T[*n].yrB = 2;
			T[*n].mat[0][3] = 't';
			T[*n].roqueB = 'e'; // aucun roque ne sera plus possible à partir de cette config
			(*n)++;
		      }
		   if ( conf->roqueB != 'p' && conf->mat[0][5] == 0 && conf->mat[0][6] == 0 )
		      if ( !caseMenaceePar( MIN, 0, 4, conf ) && !caseMenaceePar( MIN, 0, 5, conf ) && \
			   !caseMenaceePar( MIN, 0, 6, conf ) )  {
			// Faire un petit roque ...
			copier(conf, &T[*n]);
			T[*n].mat[0][4] = 0;
			T[*n].mat[0][7] = 0;
			T[*n].mat[0][6] = 'r'; T[*n].xrB = 0; T[*n].yrB = 6;
			T[*n].mat[0][5] = 't';
			T[*n].roqueB = 'e'; // aucun roque ne sera plus possible à partir de cette config
			(*n)++;

		      }
		}
			
		// vérifier les autres mouvements du roi ...
		for (i=0; i<8; i += 1) {
		   // traitement des 8 directions paires et impaires
		   a = x + D[i][0];
		   b = y + D[i][1];	  	 
		   if ( a >= 0 && a <= 7 && b >= 0 && b <= 7 ) 
			if ( conf->mat[a][b] <= 0 ) {
			   copier(conf, &T[*n]);
			   T[*n].mat[x][y] = 0;
			   T[*n].mat[a][b] = 'r'; T[*n].xrB = a; T[*n].yrB = b;
			   // cas où le roi adverse est pris...
			   if (T[*n].xrN == a && T[*n].yrN == b) { T[*n].xrN = -1; T[*n].yrN = -1; }

			   T[*n].roqueB = 'n'; // aucun roque ne sera plus possible à partir de cette config
			   (*n)++;
			}
		} // for
		break;

	}

} // deplacementsB


/* Génère les successeurs de la configuration conf dans le tableau T, 
   retourne aussi dans n le nb de configurations filles générées */
void generer_succ( struct config *conf, int mode, struct config T[], int *n )
{
	int i, j, k, stop;

	*n = 0;

	if ( mode == MAX ) {		// mode == MAX
	   for (i=0; i<8; i++)
	      for (j=0; j<8; j++)
		 if ( conf->mat[i][j] > 0 )
		    deplacementsB(conf, i, j, T, n );

	   // vérifier si le roi est en echec, auquel cas on ne garde que les succ évitants l'échec...
	   for (k=0; k < *n; k++) {
	      	i = T[k].xrB; j = T[k].yrB;  // pos du roi B dans T[k]
		// vérifier s'il est menacé dans la config T[k] ...
		if ( caseMenaceePar( MIN, i, j, &T[k] ) ) {
		    T[k] = T[(*n)-1];	// alors supprimer T[k] de la liste des succ...
		    (*n)--;
		    k--;
		}
	    } // for k
	}

	else { 				// mode == MIN
	   for (i=0; i<8; i++)
	      for (j=0; j<8; j++)
		 if ( conf->mat[i][j] < 0 )
		    deplacementsN(conf, i, j, T, n );

	   // vérifier si le roi est en echec, auquel cas on ne garde que les succ évitants l'échec...
	   for (k=0; k < *n; k++) {
		i = T[k].xrN; j = T[k].yrN;
		// vérifier s'il est menacé dans la config T[k] ...
		if ( caseMenaceePar( MAX, i, j, &T[k] ) ) {
		    T[k] = T[(*n)-1];	// alors supprimer T[k] de la liste des succ...
		    (*n)--;
		    k--;
		}
	   } // for k	
	} // if (mode == MAX) ... else ...

} // generer_succ



/***********************************************************************/
/*********** Partie:  AlphaBeta, Initialisation et affichage ***********/
/***********************************************************************/


/* Fonctions de comparaison utilisée avec qsort     */
int confcmp123(const void *a, const void *b)
{
    int x = estim((struct config *)a, MIN), y = estim((struct config *)b, MIN);
    if ( x < y )
	return -1;
    if ( x == y )
	return 0;
    return 1;
}  // fin confcmp


int confcmp321(const void *a, const void *b)
{
    int x = estim((struct config *)a, MIN), y = estim((struct config *)b, MIN);
    if ( x > y )
	return -1;
    if ( x == y )
	return 0;
    return 1;
}  // fin confcmp



/* MinMax avec elagage alpha-beta */
int minmax_ab( struct config *conf, int mode, int niv, int alpha, int beta, int auprofit, int k )
{
 	int n, i, score, score2;
 	struct config T[100];

   	if ( feuille(conf, &score) ) 
		return score;

   	if ( niv == 0 ) 
		return estim( conf, auprofit );

   	if ( mode == MAX ) {

	   generer_succ( conf, MAX, T, &n );

	   if ( k != +INFINI ) {
	   	qsort(T, n, sizeof(struct config), confcmp321);
	   	if ( k < n ) n = k; 
	   }

	   score = alpha;
	   for ( i=0; i<n; i++ ) {
   	    	score2 = minmax_ab( &T[i], MIN, niv-1, score, beta, auprofit, k );
		if (score2 > score) score = score2;
		if (score >= beta) {
			// Coupe Beta
   	      		return beta;   
	    	}
	   } 
	}
	else  { // mode == MIN 

	   generer_succ( conf, MIN, T, &n );

    	   if ( k != +INFINI ) {
	    	qsort(T, n, sizeof(struct config), confcmp123);
	   	if (k < n ) n = k;
	   }

	   score = beta;
	   for ( i=0; i<n; i++ ) {
   	    	score2 = minmax_ab( &T[i], MAX, niv-1, alpha, score, auprofit, k );
		if (score2 < score) score = score2;
		if (score <= alpha) {
			// Coupe Alpha
   	      		return alpha;   
	    	}
	   }
	}

        if ( score == +INFINI ) score = +100;
        if ( score == -INFINI ) score = -100;

	return score;

} // minmax_ab


/* Intialise la disposition des pieces dans la configuration initiale conf */
void init( struct config *conf )
{
   	int i, j;

    	for (i=0; i<8; i++)
		for (j=0; j<8; j++)
			conf->mat[i][j] = 0;	// Les cases vides sont initialisées avec 0

	conf->mat[0][0]='t'; conf->mat[0][1]='c'; conf->mat[0][2]='f'; conf->mat[0][3]='n';
	conf->mat[0][4]='r'; conf->mat[0][5]='f'; conf->mat[0][6]='c'; conf->mat[0][7]='t';

	for (j=0; j<8; j++) {
		conf->mat[1][j] = 'p';
 		conf->mat[6][j] = -'p'; 
		conf->mat[7][j] = -conf->mat[0][j];
	}

	conf->xrB = 0; conf->yrB = 4;
	conf->xrN = 7; conf->yrN = 4;

	conf->roqueB = 'r';
	conf->roqueN = 'r';

	conf->val = 0;

} // init


void formuler_coup( struct config *oldconf, struct config *newconf, char *coup )
{
   	int i,j;
	char piece[20];

	// verifier si roqueB effectué ...
	if ( newconf->roqueB == 'e' && oldconf->roqueB != 'e' ) {
	   if ( newconf->yrB == 2 ) sprintf(coup, "g_roqueB" ); 
	   else  sprintf(coup, "p_roqueB" ); 
	   return;
	}

	// verifier si roqueN effectué ...
	if ( newconf->roqueN == 'e' && oldconf->roqueN != 'e' ) {
	   if ( newconf->yrN == 2 ) sprintf(coup, "g_roqueN" ); 
	   else  sprintf(coup, "p_roqueN" ); 
	   return;
	}

	for(i=0; i<8; i++)  
	   for (j=0; j<8; j++)
		if ( oldconf->mat[i][j] != newconf->mat[i][j] ) 
		   if ( newconf->mat[i][j] != 0 ) {
			switch (newconf->mat[i][j]) {
			   case -'p' : sprintf(piece,"pionN"); break;
			   case 'p' : sprintf(piece,"pionB"); break;
			   case -'c' : sprintf(piece,"cavalierN"); break;
			   case 'c' : sprintf(piece,"cavalierB"); break;
			   case -'f' : sprintf(piece,"fouN"); break;
			   case 'f' : sprintf(piece,"fouB"); break;
			   case -'t' : sprintf(piece,"tourN"); break;
			   case 't' : sprintf(piece,"tourB"); break;
			   case -'n' : sprintf(piece,"reineN"); break;
			   case 'n' : sprintf(piece,"reineB"); break;
			   case -'r' : sprintf(piece,"roiN"); break;
			   case 'r' : sprintf(piece,"roiB"); break;
			}
			sprintf(coup, "%s en %c%d", piece, 'a'+j, i+1);
			return;
		   }
} // formuler_coup


/* Affiche la configuration conf */
void affich( struct config *conf, char *coup, int num )
{
	int i, j, k;
	int pB = 0, pN = 0, cB = 0, cN = 0, fB = 0, fN = 0, tB = 0, tN = 0, nB = 0, nN = 0; 

     	printf("Coup num:%3d : %s\n", num, coup);
     	printf("\n");
	for (i=0;  i<8; i++)
		printf("\t   %c", i+'a');
   	printf("\n");

	for (i=0;  i<8; i++)
		printf("\t-------");
   	printf("\n");

	for(i=8; i>0; i--)  {
		printf("    %d", i);
		for (j=0; j<8; j++) {
			if ( conf->mat[i-1][j] < 0 ) printf("\t  -%c", -conf->mat[i-1][j]);
			else if ( conf->mat[i-1][j] > 0 ) printf("\t  +%c", conf->mat[i-1][j]);
				  else printf("\t   ");
			switch (conf->mat[i-1][j]) {
			    case -'p' : pN++; break;
			    case 'p'  : pB++; break;
			    case -'c' : cN++; break;
			    case 'c'  : cB++; break;
			    case -'f' : fN++; break;
			    case 'f'  : fB++; break;
			    case -'t' : tN++; break;
			    case 't'  : tB++; break;
			    case -'n' : nN++; break;
			    case 'n'  : nB++; break;
			}
		}
		printf("\n");

		for (k=0;  k<8; k++)
			printf("\t-------");
   		printf("\n");

	}
	printf("\n\tB : p(%d) c(%d) f(%d) t(%d) n(%d) \t N : p(%d) c(%d) f(%d) t(%d) n(%d)\n\n",
		pB, cB, fB, tB, nB, pN, cN, fN, tN, nN);
	printf("\n");

} // affich





/*******************************************/
/*********** Programme principal  **********/
/*******************************************/



int main( int argc, char *argv[] )
{

   int n, i, j, score, stop, cout, hauteur, largeur, tour, Trier;
   char coup[20] = "";

   struct config T[100], conf, conf1;


   hauteur = 4;  	// par défaut on fixe la profondeur d'évaluation à 4
   largeur = +INFINI;	// et la largeur à l'infini (c-a-d le nb d'alternatives à chaque coup)

   // sinon on peut les récupérer depuis la ligne de commande
   if ( argc == 2 ) {
	hauteur = atoi( argv[1] ); 
	largeur = +INFINI;
   }
   else
	if ( argc == 3 ) {
	   hauteur = atoi( argv[1] ); 
	   largeur = atoi( argv[2] );
   	}

   if ( largeur == +INFINI ) { 
	Trier = 0;
   	printf("\n\nProfondeur d'exploration = %d\n\n", hauteur);
   }
   else {
	Trier = 1;
        printf("\n\nProfondeur d'exploration = %d  \t largeur d'exploration = %d\n\n", \
		hauteur, largeur);
   }

   // Initialise la configuration de départ
   init( &conf );
  
   printf("\n\nLes '+' : joueur maximisant et les '-' : joueur minimisant\n\n");

   // Boucle principale du déroulement d'une partie ...
   stop = 0;
   tour = MAX;	// le joueur MAX commence en premier
   while ( !stop ) {

	affich( &conf, coup, num_coup );

	if ( tour == MAX ) {
		printf("Au tour du joueur maximisant '+' "); fflush(stdout);
	    
	    	generer_succ(  &conf, MAX, T, &n );
	
	    	score = -INFINI;
	    	j = -1;
		
		if ( Trier ) {
		   qsort(T, n, sizeof(struct config), confcmp321);
		   if ( largeur < n ) n = largeur;
		}

		printf("\nnb alternatives = %d : ", n); fflush(stdout);
	    	for (i=0; i<n; i++) {
		   cout = minmax_ab( &T[i], MIN, hauteur, score, +INFINI, MAX, largeur );
		   // printf("%d ", cout); fflush(stdout);
		   printf("."); fflush(stdout);
		   if ( cout > score ) {  // Choisir le meilleur coup (c-a-d le plus grand score)
		   	score = cout;
		   	j = i;
		   }
	    	}
	    	if ( j != -1 ) { // jouer le coup et aller à la prochaine itération ...
		   // printf("\nchoix = %d (le coup num %d)\n", score, j+1);
		   printf("\n");
		   formuler_coup( &conf, &T[j], coup );
	    	   copier( &T[j], &conf );
		   conf.val = score;
	    	}
	    	else { // S'il n'y a pas de successeur possible, le joueur MAX à perdu
		   printf("\n *** le joueur maximisant '+' a perdu ***\n");
		   stop = 1;
	    	}

		tour = MIN;
	}

	else {  // donc tour == MIN
		printf("Au tour du joueur minimisant '-' "); fflush(stdout);
	    
	    	generer_succ(  &conf, MIN, T, &n );
	
	    	score = +INFINI;
	    	j = -1;
		
		if ( Trier ) {
		   qsort(T, n, sizeof(struct config), confcmp123);
		   if ( largeur < n ) n = largeur;
		}

		printf("\nnb alternatives = %d : ", n); fflush(stdout);

	    	for (i=0; i<n; i++) {
		   cout = minmax_ab( &T[i], MAX, hauteur, -INFINI, score, MIN, largeur );
		   // printf("%d ", cout); fflush(stdout);
		   printf("."); fflush(stdout);
		   if ( cout < score ) {  // Choisir le meilleur coup (c-a-d le plus petit score)
		   	score = cout;
		   	j = i;
		   }
	    	}
	    	if ( j != -1 ) { // jouer le coup et aller à la prochaine itération ...
		   // printf("\nchoix = %d (le coup num %d)\n", score, j+1);
		   printf("\n");
		   formuler_coup( &conf, &T[j], coup );
	    	   copier( &T[j], &conf );
		   conf.val = score;
	    	}
	    	else { // S'il n'y a pas de successeur possible, le joueur MIN à perdu
		   printf("\n *** le joueur minimisant '-' a perdu ***\n");
		   stop = 1;
	    	}

		tour = MAX;
	}

	num_coup++;

   } // while

}

