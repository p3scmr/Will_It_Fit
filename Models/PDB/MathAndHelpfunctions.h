double complex **ComplexArray(Dimension1, Dimension2) {
    int i; 
	int j;
    double complex ** Array;

    Array = (double complex**) malloc(Dimension1 * sizeof(double complex*));

    for (i = 0; i < Dimension1; i++) {
        Array[i] = (double complex*) malloc(Dimension2 * sizeof(double complex));

        for (j = 0; j < Dimension2; j++) {
            Array[i][j] = 0.0;
        }
    }

    return Array;
}

void FreeComplexArray(double complex **Array, int Dimension1, int Dimension2) {
    int i;

    for(i = 0; i < Dimension1; i++) {
        free(Array[i]);
    }

    free(Array);
}

void CopyResidue(struct Residue * Original, struct Residue * Copy) {
	Copy->xVolume = Original->xVolume;
	Copy->yVolume = Original->yVolume;
	Copy->zVolume = Original->zVolume;

	Copy->xXRayScattering = Original->xXRayScattering;
	Copy->yXRayScattering = Original->yXRayScattering;
	Copy->zXRayScattering = Original->zXRayScattering;

	Copy->xNeutronScattering = Original->xNeutronScattering;
	Copy->yNeutronScattering = Original->yNeutronScattering;
	Copy->zNeutronScattering = Original->zNeutronScattering;

	Copy->XRayScatteringLength    = Original->XRayScatteringLength;
	Copy->NeutronScatteringLength = Original->NeutronScatteringLength;

	Copy->Volume    = Original->Volume;
	Copy->Name[0]   = Original->Name[0];
	Copy->Name[1]   = Original->Name[1];
	Copy->Name[2]   = Original->Name[2];
	Copy->ResidueID = Original->ResidueID;
}

int Sign(double x) {
    int Result;

    if (x < 0.0) {
        Result = -1;
    } else if (x > 0.0) {
        Result = 1;
    } else {
        Result = 0;
	}

    return Result;
}

double complex PolarComplexNumber(double Radius, double Phi)
{
    if (Phi == 0)
        return Radius + I * 0.0;
    else
        return Radius * (cos(Phi) + I * sin(Phi));
}

void AddScatteringFromResidue(double complex **Beta, double q, struct Residue CurrentResidue, double Contrast, double ScatteringLengthDensityOfSolvent) {
    // Søren Kynde, 2012 (rewritten by Martin Cramer Pedersen, 2015)
    // This function adds the scattering of a residue to the scattering amplitude expandended by the spherical harmonic coefficients Beta_lm.
	int l;
	int m;
    
    double x;
    double y;
    double z;

	double Radius;
	double Theta;
	double Phi;
    
    int LegendreSize =  gsl_sf_legendre_array_n(NumberOfHarmonics);
    int LegendreIndex;
    const int LegendreMode = 3;
    double Legendre[LegendreSize];

    double Bessel[NumberOfHarmonics + 1];
    double ScatteringLengthOfResidue; 
	double ScatteringLengthOfDisplacedSolvent = CurrentResidue.Volume * ScatteringLengthDensityOfSolvent;
	double ExcessScatteringLength;

    if (Contrast < 0.0) {
       ScatteringLengthOfResidue = CurrentResidue.XRayScatteringLength;
       ExcessScatteringLength    = ScatteringLengthOfResidue - ScatteringLengthOfDisplacedSolvent;

       x = (CurrentResidue.xXRayScattering * ScatteringLengthOfResidue - CurrentResidue.xVolume * ScatteringLengthOfDisplacedSolvent) / ExcessScatteringLength;
       y = (CurrentResidue.yXRayScattering * ScatteringLengthOfResidue - CurrentResidue.yVolume * ScatteringLengthOfDisplacedSolvent) / ExcessScatteringLength;
       z = (CurrentResidue.zXRayScattering * ScatteringLengthOfResidue - CurrentResidue.zVolume * ScatteringLengthOfDisplacedSolvent) / ExcessScatteringLength;
    } else {
       ScatteringLengthOfResidue = CurrentResidue.NeutronScatteringLength;
       ExcessScatteringLength    = ScatteringLengthOfResidue - ScatteringLengthOfDisplacedSolvent;

       x = (CurrentResidue.xNeutronScattering * ScatteringLengthOfResidue - CurrentResidue.xVolume * ScatteringLengthOfDisplacedSolvent) / ExcessScatteringLength;
       y = (CurrentResidue.yNeutronScattering * ScatteringLengthOfResidue - CurrentResidue.yVolume * ScatteringLengthOfDisplacedSolvent) / ExcessScatteringLength;
       z = (CurrentResidue.zNeutronScattering * ScatteringLengthOfResidue - CurrentResidue.zVolume * ScatteringLengthOfDisplacedSolvent) / ExcessScatteringLength;
    }
    
    if(x != x|| y != y || z != z){//This is a filthy filthy hack. I feel bad.
        x = 0.0;
        y = 0.0;
        z = 0.0;
        /*if (ScatteringLengthDensityOfSolvent == 0.0)
        {
            printf("Check!\n");
        }
        else{
            printf("Nope!\n");
        }*/
        //printf("ExcessScatteringLength: %f \n", ExcessScatteringLength);
        //printf("Volume %f \n", CurrentResidue.Volume);
        //printf("ScatteringLengthOfResidue: %f \n", ScatteringLengthOfResidue);
        //printf("ScatteringLengthOfDisplacedSolvent: %f \n", ScatteringLengthOfDisplacedSolvent);
    }
    Radius = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
    Theta  = acos(z / Radius);
    Phi    = acos(x / (Radius * sin(Theta))) * Sign(y);

	// Calculate spherical Bessel functions for l = 0 to NumberOfHarmonics
    gsl_sf_bessel_jl_array(NumberOfHarmonics, q * Radius, Bessel);
    gsl_sf_legendre_array(LegendreMode, NumberOfHarmonics, cos(Theta), Legendre); //Calculate legendre polynomials P_l(cos(theta)) of degree l=m,..., up to Nh   
    for(m=0;m<=NumberOfHarmonics+1;m++){                                                               
        for(l=m;l<NumberOfHarmonics+1;l++){
            LegendreIndex = gsl_sf_legendre_array_index(l,m);
            Beta[l][m] += sqrt(4.0 * M_PI) * cpow(I,l) * ExcessScatteringLength * Bessel[l] * Legendre[LegendreIndex] * PolarComplexNumber(1.0, -m * Phi);
        }
    }
}
