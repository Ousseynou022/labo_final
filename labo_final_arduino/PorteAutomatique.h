#pragma once
#include <AccelStepper.h>

enum EtatPorte {
  FERMEE,
  OUVERTE,
  EN_FERMETURE,
  EN_OUVERTURE
};

class PorteAutomatique {
public:
  PorteAutomatique(int p1, int p2, int p3, int p4, float& distancePtr);
  void update();
  void setAngleOuvert(float angle);
  void setAngleFerme(float angle);
  void setPasParTour(int steps);
  void setDistanceOuverture(float distance);
  void setDistanceFermeture(float distance);
  const char* getEtatTexte() const;
  float getAngle() const;

private:
  AccelStepper _stepper;
  unsigned long _currentTime = 0;
  float _angleOuvert = 90;
  float _angleFerme = 0;
  int _stepsPerRev = 2048;
  float& _distance;
  float _distanceOuverture = 20;
  float _distanceFermeture = 30;
  EtatPorte _etat = FERMEE;

  void _ouvertState();
  void _fermeState();
  void _ouvertureState();
  void _fermetureState();
  void _ouvrir();
  void _fermer();
  void _mettreAJourEtat();
  long _angleEnSteps(float angle) const;
};
