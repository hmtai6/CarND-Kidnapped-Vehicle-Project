#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h>
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

// Initializes the N particles to be determined by num_particles
void ParticleFilter::init(double x, double y, double theta, double std[])
{

	// User defined number of particles => Directly impacts algorithm performance
	num_particles = 10;

	// Initializes a random generator to assist in our gaussian sampling
	default_random_engine gen;

	// Creates normal distribution for each parameter: x, y and theta
	normal_distribution<double> dist_x(x, std[0]);
	normal_distribution<double> dist_y(y, std[1]);
	normal_distribution<double> dist_theta(theta, std[2]);

	for (int i = 0; i < num_particles; i++)
	{
		Particle p;
		p.id = i;
		p.x = dist_x(gen);
		p.y = dist_y(gen);
		p.theta = dist_theta(gen);
		p.weight = 1;

		particles.push_back(p);
	}

	is_initialized = true;
}

// Predicts position of each particle given delta_t, noises, velocity and yaw rate
void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate)
{

	// Initializes a random generator to assist in our gaussian sampling
	default_random_engine gen;

	double new_theta;
	double new_x_mean;
	double new_y_mean;

	for (int i = 0; i < num_particles; i++)
	{

		if (fabs(yaw_rate) < 0.0001)
		{
			new_theta = particles[i].theta;
			new_x_mean = particles[i].x + (velocity * delta_t * cos(new_theta));
			new_y_mean = particles[i].y + (velocity * delta_t * sin(new_theta));
		}
		else
		{
			new_theta = particles[i].theta + yaw_rate * delta_t;
			new_x_mean = particles[i].x + (velocity / yaw_rate) * (sin(new_theta) - sin(particles[i].theta));
			new_y_mean = particles[i].y + (velocity / yaw_rate) * (cos(particles[i].theta) - cos(new_theta));
		}

		// Creates normal distribution for each parameter: x, y and theta
		normal_distribution<double> dist_x(new_x_mean, std_pos[0]);
		normal_distribution<double> dist_y(new_y_mean, std_pos[1]);
		normal_distribution<double> dist_theta(new_theta, std_pos[2]);

		particles[i].x = dist_x(gen);
		particles[i].y = dist_y(gen);
		particles[i].theta = dist_theta(gen);
	}
}

// Unused - to be deleted
void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs> &observations)
{
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to
	//   implement this method and use it as a helper during the updateWeights phase.
}

// Calculates weights for each individual particle
/**
 * Steps
 * 1. convert each measurement to map coordinate
 * 2. find the closest landmark of that observation
 * 3. calculate the weight for obs x 
 * 4. Accumulate weight for a particle
 * 
 **/
void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], const std::vector<LandmarkObs> &observations,
			const Map &map_landmarks)
{
	// variable to store coords converted to map scale
	LandmarkObs converted_obs;
	LandmarkObs best_landmark;

	// clears weight vector
	weights.clear();

	// Loop each particle
	for (int i = 0; i < int(particles.size()); i++)
	{
		double prob = 1.;
		// Loop all observations
		for (int k = 0; k < int(observations.size()); k++)
		{
			//  1. convert measurement to map coordinate
			LandmarkObs obs = observations[k];
			Particle part = particles[i];
			converted_obs.id = obs.id;
			converted_obs.x = obs.x * cos(part.theta) - obs.y * sin(part.theta) + part.x;
			converted_obs.y = obs.x * sin(part.theta) + obs.y * cos(part.theta) + part.y;

			double min_dist;
			//  2. find the closest landmark of that observation
			for (int m = 0; m < int(map_landmarks.landmark_list.size()); m++)
			{
				Map::single_landmark_s landmark = map_landmarks.landmark_list[m];
				double distance = dist(converted_obs.x, converted_obs.y, landmark.x_f, landmark.y_f);
				if (m == 0 || distance < min_dist) 
				{
					min_dist = distance;
					best_landmark.id 	= landmark.id_i;
					best_landmark.x 	= landmark.x_f;
					best_landmark.y 	= landmark.y_f;
				}
			}

			//  3. calculate the weight for obs x 
			const double sigma_x 	= std_landmark[0];
			const double sigma_y 	= std_landmark[1];
			const double d_x 		= converted_obs.x - best_landmark.x;
			const double d_y 		= converted_obs.y - best_landmark.y;

			double gauss_norm = (1 / (2. * M_PI * sigma_x * sigma_y));
			double exponent = -((d_x * d_x / (2 * sigma_x * sigma_x)) + (d_y * d_y / (2 * sigma_y * sigma_y)));
			double weight = gauss_norm * exp(exponent);

			//  4 Accumulate weight for a particle
			prob *= weight;
		}

		// update particle weight
		particles[i].weight = prob;

		// add into weights vector
		weights.push_back(prob);
	}
}

// Weight based resampling the particles with replacement, allowing more relevant particles to be picked more frequently
void ParticleFilter::resample()
{

	std::vector<Particle> new_particles;
	int index;

	// Initializes discrete distribution function
	std::random_device rd;
	std::mt19937 gen(rd());
	std::discrete_distribution<int> weight_distribution(weights.begin(), weights.end());

	for (int i = 0; i < num_particles; i++)
	{
		index = weight_distribution(gen);
		new_particles.push_back(particles[index]);
	}
	particles = new_particles;
}

Particle ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations,
		                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
	//particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
	// associations: The landmark id that goes along with each listed association
	// sense_x: the associations x mapping already converted to world coordinates
	// sense_y: the associations y mapping already converted to world coordinates

	//Clear the previous associations
	particle.associations.clear();
	particle.sense_x.clear();
	particle.sense_y.clear();

	particle.associations = associations;
	particle.sense_x = sense_x;
	particle.sense_y = sense_y;

	return particle;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
	copy(v.begin(), v.end(), ostream_iterator<int>(ss, " "));
	string s = ss.str();
	s = s.substr(0, s.length() - 1); // get rid of the trailing space
	return s;
}

string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
	copy(v.begin(), v.end(), ostream_iterator<float>(ss, " "));
	string s = ss.str();
	s = s.substr(0, s.length() - 1); // get rid of the trailing space
	return s;
}

string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
	copy(v.begin(), v.end(), ostream_iterator<float>(ss, " "));
	string s = ss.str();
	s = s.substr(0, s.length() - 1); // get rid of the trailing space
	return s;
}