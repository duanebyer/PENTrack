/**
 * \file
 * Contains base class TParticleSource and several dervied particle source classes.
 * Class TSource creates one of these according to user input.
 */

#ifndef SOURCE_H_
#define SOURCE_H_

#include <string>
#include <limits>
#include <random>

#include "particle.h"
#include "mc.h"

/**
 * Virtual base class for all particle sources
 */
class TParticleSource{
protected:
	double fActiveTime; ///< Duration for which the source will be active
	std::string fParticleName; ///< Name of particle that the source should create

	std::piecewise_linear_distribution<double> spectrum; ///< Parsed initial energy distribution given by user
	std::piecewise_linear_distribution<double> phi_v; ///< Parsed initial azimuthal angle distribution of velocity given by user
	std::piecewise_linear_distribution<double> theta_v; ///< Parsed initial polar angle distribution of velocity given by user
	double polarization; ///< Initial polarization of created particles
public:
	int ParticleCounter; ///< Count number of particles created by source
	/**
	 * Constructor, should be called by every derived class
	 *
	 * @param sourceconf Map of source options
	 */
	TParticleSource(std::map<std::string, std::string> &sourceconf);

	/**
	 * Destructor
	 */
	virtual ~TParticleSource(){

	}

	/**
	 * Basic creation of a new particle. Maps particle name to the corresponding class
	 *
	 * @param t Starting time
	 * @param x x coordinate of creation point
	 * @param y y coordinate of creation point
	 * @param z z coordinate of creation point
	 * @param E Initial kinetic energy
	 * @param phi Azimuthal angle of initial velocity vector
	 * @param theta Polar angle of initial velocity vector
	 * @param polarisation Initial polarisation of particle (-1, 0, 1)
	 * @param mc Random-number generator
	 * @param geometry Geometry of the simulation
	 * @param field TFieldManager containing all electromagnetic fields
	 *
	 * @return Returns newly created particle, memory has to be freed by user
	 */
	TParticle* CreateParticle(double t, double x, double y, double z, double E, double phi, double theta, double polarisation,
			TMCGenerator &mc, const TGeometry &geometry, const TFieldManager &field);


	/**
	 * Virtual routine that has to be implemented by every derived source class
	 *
	 * @return Returns newly created particle, memory has to be freed by user
	 */
	virtual TParticle* CreateParticle(TMCGenerator &mc, const TGeometry &geometry, const TFieldManager &field) = 0;
};


/**
 * Virtual base class for surface source.
 *
 * It keeps a list of STL triangles on which initial coordinates are generated
 */
class TSurfaceSource: public TParticleSource{
protected:
	double Enormal; ///< Boost given to particles starting from this surface
	std::vector<double> area_sum; ///< list of areas of corresponding triangles in TGeometry class, each entry accumulates areas of all preceding triangles that are contained in the source volume

	/**
	 * Check if point is inside the source volume.
	 *
	 * Abstract function, has to be implemented by every derived class.
	 */
	virtual bool InSourceVolume(const double x, const double y, const double z) const = 0;
public:
	/**
	 * Constructor.
	 *
	 * @param sourceconf Map of source options
	 */
	TSurfaceSource(std::map<std::string, std::string> &sourceconf):
		TParticleSource(sourceconf), Enormal(0){
		std::istringstream(sourceconf["Enormal"]) >> Enormal;
	}

	/**
	 * Create new particle on surface
	 *
	 * @return Returns newly created particle, memory has to be freed by user
	 */
	TParticle* CreateParticle(TMCGenerator &mc, const TGeometry &geometry, const TFieldManager &field);
};



/**
 * Virtual base class for volume source.
 *
 * Can generate a particle in volume source, optionally weighted by available phase space.
 * Derived sources need to implement the RandomPointInSourceVolume routine.
 */
class TVolumeSource: public TParticleSource{
private:
	/**
	 * find potential minimum in source volume
	 */
	void FindPotentialMinimum(TMCGenerator &mc, const TGeometry &geometry, const TFieldManager &field);
protected:
	double MinPot; ///< minimal potential energy in source volume
	bool fPhaseSpaceWeighting; ///< Tells source to weight particle density according to available phase space.

	/**
	 * Produce random point in the source volume
	 *
	 * Has to be implemented by every derived class
	 *
	 * @param x Returns x coordinate
	 * @param y Returns y coordinate
	 * @param z Returns z coordinate
	 * @param mc Random-number generator
	 */
	virtual void RandomPointInSourceVolume(double &x, double &y, double &z, TMCGenerator &mc) const = 0;
public:
	/**
	 * Constructor
	 *
	 * Create generic volume source
	 *
	 * @param sourceconf Map of source options
	 */
	TVolumeSource(std::map<std::string, std::string> &sourceconf):
			TParticleSource(sourceconf), MinPot(std::numeric_limits<double>::infinity()), fPhaseSpaceWeighting(false){
		std::istringstream(sourceconf["PhaseSpaceWeighting"]) >> fPhaseSpaceWeighting;
	}

	/**
	 * Create particle in source volume
	 *
	 * Particle density distribution can be weighted by available phase space
	 *
	 */
	TParticle* CreateParticle(TMCGenerator &mc, const TGeometry &geometry, const TFieldManager &field);
};

/**
 * Volume source generating points in a cuboid coordinate range
 */
class TCuboidVolumeSource: public TVolumeSource{
private:
	double xmin, xmax, ymin, ymax, zmin, zmax;

	/**
	 * Produce random point in the source volume
	 *
	 * @param x Returns x coordinate
	 * @param y Returns y coordinate
	 * @param z Returns z coordinate
	 */
	virtual void RandomPointInSourceVolume(double &x, double &y, double &z, TMCGenerator &mc) const final{
		std::uniform_real_distribution<double> unidist(0, 1);
		x = xmin + unidist(mc)*(xmax - xmin);
		y = ymin + unidist(mc)*(ymax - ymin);
		z = zmin + unidist(mc)*(zmax - zmin);
	}
public:
	/**
	 * Constructor.
	 *
	 * @param sourceconf Map of source options
	 */
	TCuboidVolumeSource(std::map<std::string, std::string> &sourceconf):
			TVolumeSource(sourceconf), xmin(0), xmax(0), ymin(0), ymax(0), zmin(0), zmax(0){
		std::istringstream(sourceconf["parameters"]) >> xmin >> xmax >> ymin >> ymax >> zmin >> zmax;
	}
};


/**
 * Volume source generating points in a cylindrical coordinate range
 */
class TCylindricalVolumeSource: public TVolumeSource{
private:
	double rmin, rmax, phimin, phimax, zmin, zmax;

	/**
	 * Produce random point in the source volume
	 *
	 * @param x Returns x coordinate
	 * @param y Returns y coordinate
	 * @param z Returns z coordinate
	 */
	void RandomPointInSourceVolume(double &x, double &y, double &z, TMCGenerator &mc) const final{
		std::linear_distribution<double> lindist(rmin, rmax);
		double r = lindist(mc); // weighting because of the volume element and a r^2 probability outwards
		std::uniform_real_distribution<double> unidist(0, 1);
		double phi_r = phimin + unidist(mc)*(phimax - phimin);
		x = r*cos(phi_r);
		y = r*sin(phi_r);
		z = zmin + unidist(mc)*(zmax - zmin);
	}
public:
	/**
	 * Constructor.
	 *
	 * @param sourceconf Map of source options
	 */
	TCylindricalVolumeSource(std::map<std::string, std::string> &sourceconf):
			TVolumeSource(sourceconf), rmin(0), rmax(0), phimin(0), phimax(0), zmin(0), zmax(0){
		std::istringstream(sourceconf["parameters"]) >> rmin >> rmax >> phimin >> phimax >> zmin >> zmax;
		phimin *= conv;
		phimax *= conv;
	}
};

/**
 * Surface source using a surface inside a cylindrial coordinate range
 */
class TCylindricalSurfaceSource: public TSurfaceSource{
private:
	double rmin, rmax, phimin, phimax, zmin, zmax;

	/**
	 * Check if a point is inside the cylindrical coordinate range
	 */
	bool InSourceVolume(const double x, const double y, const double z) const final{
		double r = sqrt(x*x + y*y);
		double phi = atan2(y, x);
		return r > rmin && r < rmax && phi > phimin && phi < phimax && z > zmin && z < zmax;
	}

	/**
	 * Check if a triangle is inside the cylidrical coordinate range
	 */
	bool InSourceVolume(const CTriangle &tri) const{
		for (int i = 0; i < 3; i++){
			if (!InSourceVolume(tri[i][0], tri[i][1], tri[i][2]))
				return false;
		}
		return true;
	}

public:
	/**
	 * Constructor.
	 *
	 * Selects all triangles from TGeometry whose vertices are inside the cylindrical coordinate range
	 *
	 * @param sourceconf Map of source options
	 * @param geometry TGeometry in which particles will be created
	 */
	TCylindricalSurfaceSource(std::map<std::string, std::string> &sourceconf, const TGeometry &geometry):
			TSurfaceSource(sourceconf), rmin(0), rmax(0), phimin(0), phimax(0), zmin(0), zmax(0){
		std::istringstream(sourceconf["parameters"]) >> rmin >> rmax >> phimin >> phimax >> zmin >> zmax;
		phimin *= conv;
		phimax *= conv;

		std::transform(	geometry.mesh.GetTrianglesBegin(),
						geometry.mesh.GetTrianglesEnd(),
						std::back_inserter(area_sum),
						[this](std::pair<CTriangle, double> tri){
							if (InSourceVolume(tri.first))
								return sqrt(tri.first.squared_area());
							return 0.;
						}
		);
		std::partial_sum(area_sum.begin(), area_sum.end(), area_sum.begin());
		std::cout << "Source area " << area_sum.back() << "m^2\n";
	}
};

/**
 * Volume source.
 *
 * Starting points are generated within an STL solid
 */
class TSTLVolumeSource: public TVolumeSource{
private:
	TTriangleMesh sourcevol; ///< internal AABB tree storing the STL solid

	/**
	 * Produce random point in the source volume
	 *
	 * @param x Returns x coordinate
	 * @param y Returns y coordinate
	 * @param z Returns z coordinate
	 */
	void RandomPointInSourceVolume(double &x, double &y, double &z, TMCGenerator &mc) const final{
		CBox bbox = sourcevol.GetBoundingBox();
		std::uniform_real_distribution<double> unidist(0, 1);
		do{
			x = bbox.xmin() + unidist(mc)*(bbox.xmax() - bbox.xmin()); // random point
			y = bbox.ymin() + unidist(mc)*(bbox.ymax() - bbox.ymin());
			z = bbox.zmin() + unidist(mc)*(bbox.zmax() - bbox.zmin());
		}while (!sourcevol.InSolid(x, y, z));
	}
public:
	/**
	 * Constructor.
	 *
	 * @param sourceconf Map of source options
	 */
	TSTLVolumeSource(std::map<std::string, std::string> &sourceconf):
			TVolumeSource(sourceconf){
		boost::filesystem::path STLfile;
		std::istringstream(sourceconf["STLfile"]) >> STLfile;
		sourcevol.ReadFile(boost::filesystem::absolute(STLfile, configpath.parent_path()).native(), 0);
	}
};


/**
 * Surface source.
 *
 * Starting points are created on a surface of the experiment geometry whose triangles are all inside the given STL solid
 */
class TSTLSurfaceSource: public TSurfaceSource{
private:
	TTriangleMesh sourcevol;
	bool InSourceVolume(const double x, const double y, const double z) const{
		return sourcevol.InSolid(x, y, z);
	}
	bool InSourceVolume(const CTriangle &tri) const{
		for (int i = 0; i < 3; i++){
			if (!sourcevol.InSolid(tri[i]))
				return false;
		}
		return true;
	}
public:
	/**
	 * Constructor.
	 *
	 * Search for all triangles in geometry's mesh which are inside the STL solid given in sourcefile.
	 *
	 * @param sourceconf Map of source options
	 * @param geometry TGeometry in which particles will be created
	 */
	TSTLSurfaceSource(std::map<std::string, std::string> &sourceconf, const TGeometry &geometry):
			TSurfaceSource(sourceconf){
		boost::filesystem::path STLfile;
		std::istringstream(sourceconf["STLfile"]) >> STLfile;
		sourcevol.ReadFile(boost::filesystem::absolute(STLfile, configpath.parent_path()).native(), 0);

		std::transform(	geometry.mesh.GetTrianglesBegin(),
						geometry.mesh.GetTrianglesEnd(),
						std::back_inserter(area_sum),
						[this](std::pair<CTriangle, double> tri){
							if (InSourceVolume(tri.first))
								return sqrt(tri.first.squared_area());
							return 0.;
						}
		);
		std::partial_sum(area_sum.begin(), area_sum.end(), area_sum.begin());
		std::cout << "Source area " << area_sum.back() << "m^2\n";
	}
};


/**
 * Create particle source as defined in config
 *
 * @param config TConfig class containing SOURCE options
 * @param geometry Geometry used in the simulation
 *
 * @return Returns Particle source as defined in config
 */
TParticleSource* CreateParticleSource(TConfig &config, const TGeometry &geometry);


#endif /* SOURCE_H_ */
