#include <Servo.h>
#include <stdint.h>

enum class ParserState
{
  awaiting_command,
  awaiting_unary_data,

  awaiting_line_end,
  awaiting_line,
}

Servo s[12];

void setup () 
{
	for (int i = 0; i < 12; ++i)
	{
		s[i].attach(i+2); // pins 0 and 1 reserved for serial communication.
	}
	Serial.begin(115200);
}

ParserState state = ParserState::ready_for_line,
void loop()
{
  switch (state)
  {
    case ParserState::awaiting_command:
    break;
    case ParserState::awaiting_unary_data:
    break;
    case ParserState::awaiting_line_end:
      while (Serial.available())
      {
        if (Serial.read() == ';')
        {
          state = ParserState::awaiting_line;
          break;
        }
      }
    break;
    case ParserState::awaiting_line:
      
    break;
  }
}

/*
unsigned int pin_index = 0;

unsigned char current_byte = 0;
uint16_t current_us = 0;
bool are_first_seven_bits_filled = false;

int read_num ()
{
  char current_byte = 0;
  int num = 0;
  
  while (current_byte != ';')
  {
    while (!Serial.available());

    current_byte = Serial.peek();
    if (current_byte == ';' || current_byte == ',') return num;
    if (current_byte < '0' || current_byte > '9') return -1;
    num *= 10;
    num += current_byte - '0';
    Serial.read();
  }

  return num;
}

int read_pin_id ()
{
  char current_byte = 0;
  int pin_id = 0;
  
  while (current_byte != ':')
  {
    while (!Serial.available());

    current_byte = Serial.read();
    if (current_byte < '0' || current_byte > '9') return -1;
    pin_id *= 10;
    pin_id += current_byte - '0';
  }

  return pin_id;
}

enum class CommandType
{
  default_all,
  write_one,
  //write_all

  invalid
};

struct Command
{
  CommandType type;
  int pin_id;
};

struct Command read_command ()
{
  char current_byte = 0;
  
  if (!Serial.available()) return Command {CommandType::invalid, -1};
  
  current_byte = Serial.peek();
  
  if (current_byte == 'c') 
  {
    Serial.read();
    return Command {CommandType::default_all, -1};
  }
  else if (current_byte > '0' && current_byte < '9')
  {
    int num = read_pin_id();
    if (num < 0 || num >= 12) return Command {CommandType::invalid, -1};
    return Command {CommandType::write_one, num};
  }
  //else * => write_all
  else
  {
    while (current_byte != ';') current_byte = Serial.read();
    return Command {CommandType::invalid, -1};
  }
}

void read_line ()
{
  Command command = read_command();
  if (command.type == CommandType::invalid) return;
  else if (command.type == CommandType::default_all)
  {
    for (int i = 0; i < 12; ++i) s[i].writeMicroseconds(1500);
  }
  else if (command.type == CommandType::write_one)
  {
    int num = read_num();
    if (num < 0) return;
    s[command.pin_id].writeMicroseconds(num);

    while (current_byte != ';') current_byte = Serial.read();
  }
  //else write_all
}

void loop()
{
	if (Serial.available() > 0) read_line();
}*/
