/*-----------------------------------------------------------------------------
Stéphane Lepoutère 10/2020

Objet contenant les valeurs en TR, avec les méthodes de
- mise à jour
- renvoie
-----------------------------------------------------------------------------*/
#ifndef VALEURS_H
#define VALEURS_H

class CValeurs
{
public:
    CValeurs(void);
    void miseAJour(float temperature, float humidite, char *timestamp);

private:
}

#endif