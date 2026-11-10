Fuse database
=============

Database created by ``tools/create_database.py`` has for each device:

* ``tilegrid.json`` with all tiles of device, and
* ``bits/<tile type>.json`` with list of fuses for each type of tile.

Tiles
-----

.. code-block:: none

  "x10y25": { "type": "plb", "x": 10, "y": 25, "rows": 31, "cols": 54,
              "start_frame": 306, "start_bit": 2486, ... }

``start_frame`` and ``start_bit`` give the place of tile in configuration memory. More tiles can
have the same ``x`` and ``y``.

Fuses
-----

.. code-block:: none

  { "name": "TOP.XI59.MC07", "type": "NONE", "frame": 0, "bit": 1, "xoff": 0, "yoff": 0,
    "map_wire_arc": 1, "remap": 0, "pll_info": 0,
    "expr": ["A", "+", "B", "+", "C", "+", "D"],
    "rpn":  ["A", "B", "+", "C", "+", "D", "+"],
    "data": { "A": "ARCVAL(S6BEG7,S1END3)", "B": "ARCVAL(S6BEG7,S2END7)",
              "C": "ARCVAL(S6BEG7,S6END5)", "D": "ARCVAL(S6BEG7,S6END7)" } }

Position
~~~~~~~~

Fuse is one bit of configuration memory:

* frame is ``start_frame`` of tile + ``frame`` of fuse,
* bit in frame is ``start_bit`` of tile + ``bit`` of fuse. Frame in bitstream has more bits than
  database knows: on EG4 6 bits are inserted at database positions 974 and 2920, so 6 is added
  when position is 974 or more, and 6 again when it is 2920 or more. Bit 0 is most significant
  bit of first byte of frame.

When ``type`` is not ``NONE`` (then ``remap`` is 1) the bit is not in tile that has the fuse, but in
tile of type ``type`` at ``x + xoff``, ``y + yoff``, and ``start_frame`` and ``start_bit`` of that
tile are used.

When ``pll_info`` is 1 fuse is not in configuration memory, but in PLL configuration data
(see :doc:`bitstream_format`).

Same name can be used for more than one fuse of tile type.

Expression
~~~~~~~~~~

Fuse is set when its expression is true. ``expr`` is expression as written by vendor and ``rpn``
is the same in reverse Polish notation. Letters are terms from ``data``, and operators are:

+----------+-------------+
| Operator | Meaning     |
+==========+=============+
| ``+``    | or          |
+----------+-------------+
| ``*``    | and         |
+----------+-------------+
| ``~``    | not         |
+----------+-------------+

Terms are:

+--------------------------------+--------------------------------------------------------------------+
| Term                           | True when                                                          |
+================================+====================================================================+
| ``ARCVAL(dst,src)``            | wire ``dst`` of this tile is driven from wire ``src``              |
+--------------------------------+--------------------------------------------------------------------+
| ``WIRE(name)``                 | wire ``name`` of this tile is used by any net                      |
+--------------------------------+--------------------------------------------------------------------+
| ``PROPERTY(site.name,value)``  | attribute ``name`` of cell placed on ``site`` has this value       |
+--------------------------------+--------------------------------------------------------------------+
| ``MEMORY(site,name,n)``        | bit ``n`` of attribute ``name`` (number) of cell on ``site`` is 1  |
+--------------------------------+--------------------------------------------------------------------+
| ``SELECTOR(site,name,n)``      | same as ``MEMORY``                                                 |
+--------------------------------+--------------------------------------------------------------------+
| ``COMP(site)``                 | there is cell placed on ``site``                                   |
+--------------------------------+--------------------------------------------------------------------+
| ``PORTS(site.name,port)``      | not known, see below                                               |
+--------------------------------+--------------------------------------------------------------------+
| ``FALSE``, ``TRUE``            | constant                                                           |
+--------------------------------+--------------------------------------------------------------------+

Names of sites are ``SLICE0`` to ``SLICE3``, ``EMB0`` to ``EMB3``, ``EMB32K0``, ``MULT18_0``,
``IOL0``, ``PLL``, ``CFG``... Number is the last number of location TD gives to cell
(``x16y27_2`` is site 2 of tile at x16y27).

Attributes are the ones TD writes in physical netlist (``write_pnl``), with some differences:

* LUT content is written there as equation (``LUTF0=(C@D)``, where ``@`` is xor) or as
  ``INIT_LUTF0=16'hA55A``. Input A is bit 0 of index in table, B is bit 1, C is bit 2, D is bit 3.
* For ``mslice`` in ``MODE:RIPPLE`` content of LUT is not written, it depends on ``ALUTYPE``:

  +------------------+--------+--------+
  | ALUTYPE          | LUT0   | LUT1   |
  +==================+========+========+
  | ADD              | 666A   | 666A   |
  +------------------+--------+--------+
  | ADD_CARRY        | 000A   | 666A   |
  +------------------+--------+--------+
  | SUB              | 999A   | 999A   |
  +------------------+--------+--------+
  | SUB_CARRY        | 0005   | 999A   |
  +------------------+--------+--------+
  | A_LE_B           | 999C   | 999C   |
  +------------------+--------+--------+
  | A_LE_B_CARRY     | 000A   | 999C   |
  +------------------+--------+--------+

* Net can be routed through LUT that design does not use (route ``a6`` to ``fx6``). There is no
  cell for it, and it works as cell with ``LUT0=(A)`` for slice 0 and 1 (``A`` is the input that
  is used). For slice 2 and 3 it is
  ``LUTF0=(A)`` when route goes to ``f``, and ``LUTG0=(A)`` with ``LSFXMUX0:LUTG`` when it goes to
  ``fx``. Number of LUT in slice is number of wire modulo 2.
* PLL dividers ``REFCLK_DIV``, ``FBCLK_DIV``, ``CLKCn_DIV`` and also ``ICP_CURRENT`` are kept as
  value - 1, and ``CLKCn_FPHASE`` as (value + 5) modulo 8.
* ``PROPERTY`` of attribute that is number compares it written as 16 binary digits
  (``PROPERTY(PLL.CLKC0_FPHASE,0000000000000001)``).

Wires
~~~~~

Wire that goes over more tiles has different name in each of them: ``N2BEG4`` in tile where it is
driven, ``N2MID4`` in next tile to the north and ``N2END4`` in tile after that. Wires of length 6
have ``MID`` after 3 tiles. Switch is always in tile where wire begins, and source is named as
it is seen in that tile.

Wire that would leave the chip comes back in opposite direction, and has name of that direction:
``N2BEG4`` of tile in row 70 of device with rows 0 to 71 is ``N2MID4`` in row 71, and it ends in the
same tile as ``S2END4``. Both names can have switches there, and they are different fuses.

Some connections of clock network in netlist of TD have no ``ARCVAL`` term (for example ``GCLK0``
from ``VPBX0T``, or ``CLK_S0`` from ``CLK0``). They have no fuse of their own, fuses there depend on
``WIRE`` terms.

Checking
--------

Three scripts in ``tools`` compare database with what TD does. For all of them the design is
placed and routed in TD, and then:

.. code-block:: none

  write_pnl design.pnl
  bitgen -bit design.bit -info -log_file design.log
  bitgen -bit design2.bit -info -attr_info -log_file design.attr.log

* ``check_fuse_creation.py design.bit design.log <db_dir>`` checks position of fuses: every fuse
  TD names in log has to be set in bitstream, and no other bit can be set.
* ``check_fuse_expressions.py design.attr.log <db_dir>`` checks expressions: with ``-attr_info``
  TD writes for each fuse what made it set, each such reason has to be term of expression, and
  expression has to be true with only these terms true.
* ``predict_fuses.py design.pnl design.bit <db_dir>`` evaluates all expressions of all tiles for
  design in physical netlist, and compares result with bitstream.

Checked with TD 5.6.5 on EG4S20NG88 (``eagle_s20``) with seven designs. Together they use slices
in modes ``LOGIC``, ``RIPPLE``, ``DPRAM`` and ``RAMW``, block RAM (``DP8K`` and ``DP16K``), multiplier,
PLL, global clocks, and LVCMOS inputs, outputs and bidirectional pins. All three scripts find no
difference for any of them; the largest is SoC with 196157 bits of configuration memory set.

Not checked
~~~~~~~~~~~

* ``PORTS`` term (20 fuses of ``plb``, for inputs E and MI of LUT in slices 2 and 3) was never
  given by TD as reason, also when these inputs were connected. It was taken as false.
* Few expressions have ``(`` without ``)`` (``A+B+(C`` for ``MC1_TRIMCn_BYPASS_N`` of PLL, and
  only ``(`` for ``F_ESCLKINV_EMBn``), this is how they are in files of vendor.
* Tile types where test designs set no fuse: ``adc``, ``osc``, ``clkdiv``, ``ioclk``, ``gclk_csb``,
  ``gclk_fclk``, ``ios_bankref``, ``pwrmnt``, ``por``. Also other IO standards and IO registers.
* Other devices.
