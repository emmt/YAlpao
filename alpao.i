/*
 * alpao.i --
 *
 * Yorick interface to Alpao deformable mirrors.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 2019: Éric Thiébaut <eric.thiebaut@univ-lyon1.fr>
 *
 * See LICENSE.md for details.
 *
 */

if (is_func(plug_in)) plug_in, "yalpao";

extern alpao_open;
/* DOCUMENT dm = alpao_open(config);

     Open device to Alpao deformable mirror.  CONFIG is the path to the
     device configuration file (with an optional ".acfg" extension).
     The device is automatically closed when DM is no longer referenced.

     Object DM can be used as follows to retrieve parameters.

         val = dm.key;
         val = dm("key");

     where "key" (ignoring the case of the characters) is the name of the
     parameter.   For instance:

         dm.NbOfActuator
         dm("NbOfActuator")
         dm.nbofactuator
         dm("nbofactuator")

     all yields the number of actuators.

     Object DM can be used as a function/subroutine to send a command to the
     deformable mirror:

         dm, x;      // send command x
         xp = dm(x); // send command x and return actual command

     where X is a vector of DM.num values and XP is X clipped to the limits
     accepted by the deformable mirror.

     To retrieve the last commands sent to the deformable mirror:

         xp = dm();

   SEE ALSO: alpao_reset, alpao_stop, alpao_get, alpao_set.
*/

extern alpao_reset;
/* DOCUMENT alpao_reset, dm;

     Reset the shape of the deformable mirror DM.  The argument is returned
     if called as a function.

   SEE ALSO: alpao_open, alpao_stop.
 */

extern alpao_stop;
/* DOCUMENT alpao_stop, dm;

     Stop controlling the shape of the deformable mirror DM.  The argument is
     returned if called as a function.

   SEE ALSO: alpao_open, alpao_reset.
 */
extern alpao_get;
extern alpao_set;
/* DOCUMENT val = alpao_get(dm, key);
         or alpao_set, dm, key, val;

     Get/set the value of a parameter of the deformable mirror DM.  Argument
     KEY is the name of the parameter (ignoring the case of the characters).

   SEE ALSO: alpao_open, , alpao_reset, alpao_stop.
 */

/*---------------------------------------------------------------------------*/
