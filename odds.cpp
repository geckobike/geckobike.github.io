#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <time.h>
#include <math.h>
#include <map>

#include "mtrand.cpp"


MTRand53 g_random;

struct Simulation
{
	struct Death
	{
		bool helmet;
		bool certain;
		bool headDeath;
		bool control;
	};

	int population;
	float wearingFraction;
	float probabilityOfCrashing[2];

	float probabilityOfDyingRegardless[2];
	float probabilityOfFatalHeadInjury[2]; // without helmet

	float helmetEffectiveness;

	Simulation()
	{
		population = (int)(0.001e6);
		wearingFraction = 0.25;

		probabilityOfCrashing[0] = 1.0/1.0; // helmet = 0
		probabilityOfCrashing[1] = probabilityOfCrashing[0] * 2.0;

		// Probablity, upon crashing!
		probabilityOfDyingRegardless[0] = 1.0/10.0;
		probabilityOfFatalHeadInjury[0] = 1.0/5.0;

		probabilityOfDyingRegardless[1] = probabilityOfDyingRegardless[0] * 2.0;
		probabilityOfFatalHeadInjury[1] = probabilityOfFatalHeadInjury[0] * 2.0;

		helmetEffectiveness = 0.3;
	}

	float Run(bool bPrint, float coronerAccuracy)
	{
		//died.reserve((int)((float)population * probabilityOfCrashing) * (2010-2006));
		std::vector<Death> deaths;
		
		//for (int y=2006; y<=2010; y++)
		{
			//wearingFraction = 0.25 + (0.35-0.25)*(float)(y-2006)/(float)(2010-2006);

			for (int i=0; i<population; i++)
			{
				int helmet = (g_random() <= wearingFraction) ? 1 : 0;
				bool crash = (g_random() <= probabilityOfCrashing[helmet]);
				if (crash)
				{
					bool certainDeath = g_random() <= probabilityOfDyingRegardless[helmet];
					bool headDeath = g_random() <= probabilityOfFatalHeadInjury[helmet];
					// Helmet saves?
					if (helmet && (g_random() < helmetEffectiveness))
						headDeath = false;

					if (headDeath || certainDeath)
					{
						Death d;
						d.helmet = helmet!=0;
						d.certain = certainDeath;
						d.headDeath = headDeath;
						d.control = certainDeath;
						deaths.push_back(d);
					}
				}
			}
		}

		// Accumulate results
		float died[2] = {0,0};
		float diedControl[2] = {0,0};
		float diedHeadOnly[2] = {0,0};

		for (int i=0; i<(int)deaths.size(); i++)
		{
			Death& d = deaths[i];
			int helmet = d.helmet ? 1 : 0;
			died[helmet] += 1.0;

			bool bControl = d.certain!=0 && !d.headDeath;

			// Less likely to be identified as a control if not wearing a helmet
			if (!d.helmet && g_random() > coronerAccuracy)
				bControl = false;
			
			if (d.helmet && g_random() > coronerAccuracy)
				bControl = true;

			if (bControl)
			{
				diedControl[helmet] += 1.0;
			}
			else
			{
				diedHeadOnly[helmet] += 1.0;
			}
		}

		if (bPrint)
		{
			printf("# # # # Results:\n");
			printf("# # # #            helmet     no-helmet\n");
			printf("# # # # died         %f          %f    total: %f\n", died[1], died[0], (died[0]+died[1]));
			printf("# # # # control      %f          %f    total: %f\n", diedControl[1], diedControl[0], (diedControl[0]+diedControl[1]));
			printf("# # # # head-only    %f          %f    total: %f\n", diedHeadOnly[1], diedHeadOnly[0], (diedHeadOnly[0]+diedHeadOnly[1]));
			printf("# # # # \n");
			printf("# # # # control wearing fraction: %f\n", diedControl[1]/(diedControl[1]+diedControl[0]));
		}

		// Odds ratio:
		//  = (diedHelmet/diedNoHelmet) / (controlHelmet/controlNoHelmet)
		float oddsRatio_died = ((float)died[1]/(float)died[0])/((float)diedControl[1]/(float)diedControl[0]);
		if( bPrint)
			printf("# # # # oddsRatio (died) = %f\n", oddsRatio_died);
		
		float oddsRatio_headOnly = ((float)diedHeadOnly[1]/(float)diedHeadOnly[0])/((float)diedControl[1]/(float)diedControl[0]);
		if( bPrint)
			printf("# # # # oddsRatio (headOnly) = %f\n", oddsRatio_headOnly);

		fflush(stdout);
		
		//oddsRatio_headOnly = ((float)diedHeadOnly[1]/(float)diedHeadOnly[0]) * (1.f-wearingFraction)/wearingFraction;

		return oddsRatio_headOnly;
	}
};

double sqr(double x) { return x*x; }


int main()
{
	// Seed the MT rand generator
	time_t now;
   	time(&now);
	g_random.seed((long)now);
	
	// result buckets
	std::map<int, int> buckets;
	const float resolution = 0.01f;

	for (int profile=0; profile<2; ++profile)
	{
		// Zero buckets
		for (int i=0; i<(int)((2.0+resolution)/resolution); i++)
		{
			buckets[i] = 0;
		}

		float coronerAccuracy = (profile==0) ? 1.0f : 0.9f;

		for (int repeat=0; repeat<5; repeat++)
		{
			const int N = 10000;
			float results[N];
			for (int i=0; i<N; i++)
			{
				Simulation simulation;
				results[i] = simulation.Run(true, coronerAccuracy);
			}
			float av = 0.f;
			for (int i=0; i<N; i++)
			{
				av += results[i];
			}
			av /= (float)N;
			float standardDeviation = 0.f;
			for (int i=0; i<N; i++)
			{
				standardDeviation += sqr(results[i]-av);
			}
			standardDeviation = sqrt(standardDeviation/float(N));
			printf("# # # # result average: %f  SD: %f\n", av, standardDeviation);

			for (int i=0; i<N; i++)
			{
				int bucket = (int)((results[i]/resolution)+0.5f);
				buckets[bucket]++;
			}

		}

		// Print the buckets
		for (int i=0; i<(int)((2.0+resolution)/resolution); i++)
		{
			printf("%f %d\n", (float)i*resolution, (int)buckets[i]);
		}
	}

	return 0;
}
