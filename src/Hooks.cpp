////////////////////////////////////////////////////////////////////////////////
// task - a command line task list manager.
//
// Copyright 2006 - 2010, Paul Beckingham.
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the
//
//     Free Software Foundation, Inc.,
//     51 Franklin Street, Fifth Floor,
//     Boston, MA
//     02110-1301
//     USA
//
////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include "Context.h"
#include "Hooks.h"

extern Context context;

////////////////////////////////////////////////////////////////////////////////
Hooks::Hooks ()
{
}

////////////////////////////////////////////////////////////////////////////////
Hooks::~Hooks ()
{
}

////////////////////////////////////////////////////////////////////////////////
void Hooks::initialize ()
{
  api.initialize ();

  // TODO Enumerate all hooks, and tell API about the script files it must load
  //      in order to call them.  Note that API will perform a deferred read,
  //      which means that if it isn't called, a script will not be loaded.

  std::vector <std::string> vars;
  context.config.all (vars);

  std::vector <std::string>::iterator it;
  for (it = vars.begin (); it != vars.end (); ++it)
  {
    std::string type;
    std::string name;
    std::string value;

    // "<type>.<name>"
    Nibbler n (*it);
    if (n.getUntil ('.', type) &&
        type == "hook"         &&
        n.skip ('.')           &&
        n.getUntilEOS (name))
    {
      std::string value = context.config.get (*it);
      Nibbler n (value);

      // <path>:<function> [, ...]
      while (!n.depleted ())
      {
        std::string file;
        std::string function;
        if (n.getUntil (':', file) &&
            n.skip (':')           &&
            n.getUntil (',', function))
        {
          context.debug (std::string ("Event '") + name + "' hooked by " + file + ", function " + function);
          Hook h (name, Path::expand (file), function);
          all.push_back (h);

          (void) n.skip (',');
        }
        else
          throw std::string ("Malformed hook definition '") + *it + "'";
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
bool Hooks::trigger (const std::string& event)
{
#ifdef HAVE_LIBLUA
  std::vector <Hook>::iterator it;
  for (it = all.begin (); it != all.end (); ++it)
  {
    if (it->event == event)
    {
      bool rc = true;
      std::string type;
      if (eventType (event, type))
      {
        // TODO Figure out where to get the calling-context info from.
             if (type == "program") rc = api.callProgramHook (it->file, it->function);
        else if (type == "list")    rc = api.callListHook    (it->file, it->function/*, tasks*/);
        else if (type == "task")    rc = api.callTaskHook    (it->file, it->function, 0);
        else if (type == "field")   rc = api.callFieldHook   (it->file, it->function, "field", "value");
      }
      else
        throw std::string ("Unrecognized hook event '") + event + "'";

      // If any hook returns false, stop.
      if (!rc)
        return false;
    }
  }
#endif

  return true;
}

////////////////////////////////////////////////////////////////////////////////
bool Hooks::eventType (const std::string& event, std::string& type)
{
  if (event == "post-start"   ||
      event == "pre-exit"     ||
      event == "pre-dispatch" || event == "post-dispatch")
  {
    type = "program";
    return true;
  }
  else if (event == "?")
  {
    type = "list";
    return true;
  }
  else if (event == "?")
  {
    type = "task";
    return true;
  }
  else if (event == "?")
  {
    type = "field";
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
