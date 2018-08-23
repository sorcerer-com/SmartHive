#ifndef DATASAVER_H
#define DATASAVER_H

#include <EEPROM.h>

class DataSaverClass
{
  private:
    const int dataAllocation = 100;
    const int maxSize = 800;

    struct
    {
      int start;
      int size;
      int version;
      int sleepTime; // in seconds
    } data;

  public:
    void setVersion(const int& value) // in seconds
    {
      if (data.version == value)
        return;

      data.version = value;
      EEPROM.put(0, data);
      EEPROM.commit();

      /*
        Serial.print("DataSaver set version: ");
        Serial.println(value);
        //*/
    }

    inline int getVersion()
    {
      return data.version;
    }

    void setSleepTime(const int& value) // in seconds
    {
      if (data.sleepTime == value)
        return;

      data.sleepTime = value;
      EEPROM.put(0, data);
      EEPROM.commit();

      /*
        Serial.print("DataSaver set sleep time: ");
        Serial.println(value);
        //*/
    }

    inline int getSleepTime()
    {
      return data.sleepTime;
    }

  public:
    void init()
    {
      EEPROM.begin(4096);
      data.start = 0;
      data.size = 0;
      data.version = 0;
      data.sleepTime = 0;

      // if cannot read the EEPROM
      if (EEPROM.getDataPtr() == NULL)
      {
        data.start = -1;
        return;
      }

      // if the start and size are not empty
      if (EEPROM.read(0) != 255 && EEPROM.read(1) != 255)
        EEPROM.get(0, data);
    }

    inline int size()
    {
      return data.size;
    }

    inline bool isInit()
    {
      return data.size != -1;
    }

    inline bool isFull()
    {
      return data.size == maxSize;
    }

    inline boolean isEmpty()
    {
      return data.size == 0;
    }

    bool enqueue(const float& value, const bool& commit = true)
    {
      if (data.start == -1)
        return false;

      // if the queue is full remove item to add the new one
      if (isFull())
      {
        float temp;
        dequeue(temp);
      }

      int idx = (data.start + data.size) % maxSize;
      data.size++;
      EEPROM.put(0, data);
      EEPROM.put(dataAllocation + idx * sizeof(value), value);
      if (commit)
        EEPROM.commit();

      /*
        Serial.print("DataSaver enqueue");
        Serial.print(" start: ");
        Serial.print(data.start);
        Serial.print(" size: ");
        Serial.print(data.size);
        Serial.print(" Address: ");
        Serial.print(dataAllocation + idx * sizeof(value));
        Serial.print(" Value: ");
        Serial.println(value);
        //*/
      return true;
    }

    bool dequeue(float& value, const bool& commit = true)
    {
      if (data.start == -1 || isEmpty())
        return false;

      EEPROM.get(dataAllocation + data.start * sizeof(value), value);
      data.start = (data.start + 1) % maxSize;
      data.size--;
      EEPROM.put(0, data);
      if (commit)
        EEPROM.commit();

      /*
        Serial.print("DataSaver dequeue");
        Serial.print(" start: ");
        Serial.print(data.start);
        Serial.print(" size: ");
        Serial.print(data.size);
        Serial.print(" Address: ");
        Serial.print(dataAllocation + data.start * sizeof(value));
        Serial.print(" Value: ");
        Serial.println(value);
        //*/
      return true;
    }

    inline void commit()
    {
      EEPROM.commit();
    }

    bool getItem(const int& index, float& value)
    {
      if (data.start == -1 && isEmpty())
        return false;

      int idx = (data.start + index) % maxSize;
      EEPROM.get(dataAllocation + idx * sizeof(value), value);
      return true;
    }

    void print()
    {
      Serial.println("Saved Data:");
      float value;
      for (int i = 0; i < data.size; i++)
      {
        int idx = (data.start + i) % maxSize;
        EEPROM.get(dataAllocation + idx * sizeof(value), value);
        Serial.println(value);
      }
    }
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
DataSaverClass DataSaver;
#endif

#endif // DATASAVER_H
