#ifndef DATASAVER_H
#define DATASAVER_H

#include <EEPROM.h>

class DataSaverClass
{
  private:
    const int maxCount = 800;
    int nextFreeSlot;

  public:
    void init()
    {
      EEPROM.begin(4096);
      nextFreeSlot = 0;

      // if cannot read the EEPROM
      if (EEPROM.getDataPtr() == NULL)
      {
        nextFreeSlot = -1;
        return;
      }

      for (int i = 0; i < 4096; i++)
      {
        if (EEPROM.read(i) == 255)
        {
          nextFreeSlot = i;
          break;
        }
      }
    }

    inline int count() const
    {
      return nextFreeSlot;
    }

    bool save(const float& value)
    {
      if (nextFreeSlot == -1 || nextFreeSlot >= maxCount)
        return false;
      /*
        Serial.print("EEPROM Save");
        Serial.print(" nextFreeSlot: ");
        Serial.print(nextFreeSlot);
        Serial.print(" Address: ");
        Serial.print(maxCount + nextFreeSlot * sizeof(value));
        Serial.print(" Value: ");
        Serial.println(value);
        //*/
      EEPROM.write(nextFreeSlot, 1);
      EEPROM.write(nextFreeSlot + 1, 255); // mark next as empty
      EEPROM.put(maxCount + nextFreeSlot * sizeof(value), value);
      EEPROM.commit();
      nextFreeSlot++;
      return true;
    }

    bool load(const int& index, float& value) const
    {
      if (nextFreeSlot == -1 || index >= nextFreeSlot)
        return false;

      EEPROM.get(maxCount + index * sizeof(value), value);
      /*
        Serial.print("EEPROM Load");
        Serial.print(" index: ");
        Serial.print(index);
        Serial.print(" Address: ");
        Serial.print(maxCount + index * sizeof(value));
        Serial.print(" Value: ");
        Serial.println(value);
        //*/
      return true;
    }

    bool clear()
    {
      if (nextFreeSlot == -1)
        return false;

      nextFreeSlot = 0;
      EEPROM.write(nextFreeSlot, 255);
      EEPROM.commit();
      return true;
    }

    void print()
    {
      Serial.println("Saved Data:");
      float value;
      for (int i = 0; i < count(); i++)
      {
        load(i, value);
        Serial.println(value);
      }
      Serial.println();
    }
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
DataSaverClass DataSaver;
#endif

#endif // DATASAVER_H

