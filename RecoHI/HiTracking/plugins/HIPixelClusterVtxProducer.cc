#include "HIPixelClusterVtxProducer.h"

#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/Event.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetUnit.h"
#include "Geometry/TrackerTopology/interface/RectangularPixelTopology.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/CommonDetUnit/interface/GeomDet.h"

#include "DataFormats/TrackerRecHit2D/interface/SiPixelRecHitCollection.h"

#include "DataFormats/GeometryVector/interface/LocalPoint.h"
#include "DataFormats/GeometryVector/interface/GlobalPoint.h"

#include "DataFormats/SiPixelDetId/interface/PXBDetId.h"

#include "SimDataFormats/Vertex/interface/SimVertexContainer.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>

/*****************************************************************************/
HIPixelClusterVtxProducer::HIPixelClusterVtxProducer(const edm::ParameterSet& ps)
  : srcPixels_(ps.getUntrackedParameter<std::string>("pixelRecHits","siPixelRecHits")),
    minZ_(ps.getUntrackedParameter<double>("minZ",-15.9)),
    maxZ_(ps.getUntrackedParameter<double>("maxZ",15.95)),
    zStep_(ps.getUntrackedParameter<double>("zStep",0.1))

{
  // Constructor
  produces<reco::VertexCollection>();
}


/*****************************************************************************/
HIPixelClusterVtxProducer::~HIPixelClusterVtxProducer()
{ 
  // Destructor
}


/*****************************************************************************/
void HIPixelClusterVtxProducer::produce(edm::Event& ev, const edm::EventSetup& es)
{

  // new vertex collection
  std::auto_ptr<reco::VertexCollection> vertices(new reco::VertexCollection);

  // get pixel rechits
  edm::Handle<SiPixelRecHitCollection> hRecHits;
  try {
    ev.getByLabel(edm::InputTag(srcPixels_),hRecHits);
  } catch (...) {}

  // get tracker geometry
  if (hRecHits.isValid()) {
    edm::ESHandle<TrackerGeometry> trackerHandle;
    es.get<TrackerDigiGeometryRecord>().get(trackerHandle);
    const TrackerGeometry *tgeo = trackerHandle.product();
    const SiPixelRecHitCollection *hits = hRecHits.product();

    // loop over pixel rechits
    std::vector<VertexHit> vhits(hits->dataSize());
    for(SiPixelRecHitCollection::DataContainer::const_iterator hit = hits->data().begin(), 
          end = hits->data().end(); hit != end; ++hit) {
      if (!hit->isValid())
        continue;
      DetId id(hit->geographicalId());
      if(id.subdetId() != int(PixelSubdetector::PixelBarrel))
        continue;
      const PixelGeomDetUnit *pgdu = static_cast<const PixelGeomDetUnit*>(tgeo->idToDet(id));
      if (1) {
        const RectangularPixelTopology *pixTopo = 
          static_cast<const RectangularPixelTopology*>(&(pgdu->specificTopology()));
	std::vector<SiPixelCluster::Pixel> pixels(hit->cluster()->pixels());
        bool pixelOnEdge = false;
        for(std::vector<SiPixelCluster::Pixel>::const_iterator pixel = pixels.begin(); 
            pixel != pixels.end(); ++pixel) {
          int pixelX = pixel->x;
          int pixelY = pixel->y;
          if(pixTopo->isItEdgePixelInX(pixelX) || pixTopo->isItEdgePixelInY(pixelY)) {
            pixelOnEdge = true;
            break;
          }
        }
        if (pixelOnEdge)
          continue;
      }

      LocalPoint lpos = LocalPoint(hit->localPosition().x(),
                                   hit->localPosition().y(),
                                   hit->localPosition().z());
      GlobalPoint gpos = pgdu->toGlobal(lpos);
      VertexHit vh;
      vh.z = gpos.z(); 
      vh.r = gpos.perp(); 
      vh.w = hit->cluster()->sizeY();
      vhits.push_back(vh);
    }

    // estimate z-position from cluster lengths
    double zest = 0.0;
    int nhits = 0, nhits_max = 0;
    double chi = 0, chi_max = 1e+9;
    for(double z0 = minZ_; z0 <= maxZ_; z0 += zStep_) {
      nhits = getContainedHits(vhits, z0, chi);
      if(nhits == 0) 
	continue;
      if(nhits > nhits_max) { 
	chi_max = 1e+9; 
	nhits_max = nhits; 
      }
      if(nhits >= nhits_max && chi < chi_max) {
	chi_max = chi; 
	zest = z0;   
      }
    } 

    LogTrace("MinBiasTracking")
      << "  [vertex position] estimated = " << zest 
      << " | pixel barrel hits = " << hits->size();

    // put 1-d vertex and dummy errors into collection
    reco::Vertex::Error err;
    err(2,2) = 0.6 * 0.6;
    reco::Vertex ver(reco::Vertex::Point(0,0,zest), err, 0, 1, 1);
    vertices->push_back(ver);
  }

  ev.put(vertices);
}

/*****************************************************************************/
int HIPixelClusterVtxProducer::getContainedHits(const std::vector<VertexHit> &hits, double z0, double &chi)
{
  // Calculate number of hits contained in v-shaped window in cluster y-width vs. z-position.

  int n = 0;
  chi   = 0.;

  for(std::vector<VertexHit>::const_iterator hit = hits.begin(); hit!= hits.end(); hit++) {
    double p = 2 * fabs(hit->z - z0)/hit->r + 0.5; // FIXME
    if(TMath::Abs(p - hit->w) <= 1.) { 
      chi += fabs(p - hit->w);
      n++;
    }
  }
  return n;
}
