# Future TODO

## Mass / specific heat migration — remaining work

We did the first pass (Weight→Mass, HeatCapacity→SpecificHeat, C = m×c everywhere). **Still to do:**

- **Save format**: Old saves have `Weight` (int) and possibly particle `heatCapacity`. Add load migration: if element has "Weight" in save, convert to Mass (e.g. value/1000.0f); if particle has heatCapacity, set specificHeat = heatCapacity / element_Mass or drop and use element default.
- **Lua**: compat.lua now maps `weight` → `Mass`. Check any other Lua API that exposes Weight/HeatCapacity by name (particle props: we now have `specificHeat`; element props: `Mass`, `SpecificHeat`). Document in Lua docs.
- **Real masses per element**: Right now Mass = old_Weight/1000.0f everywhere. Set real values from density (e.g. water 0.1 kg, tungsten 1.93 kg, gases 0.0001 kg) for key elements; then gradually for the rest.
- **Element-specific gameplay**: HCL/BLOD swap formula uses `Mass - pow(Mass,2)/10` — was tuned for old 1–100 scale; may need retuning. STKM momentum divisor (1000), WEB threshold (0.05f) — tune so behaviour feels right.
- **Docs**: SIMULATION_UNITS.md still says "Weight" in places; update to Mass/specific heat. MISSING_ITEMS.md, AIR_SOLVER_IMPROVEMENT_PLAN.md, TPT_AIR_REPLACEMENT_PLAN.md: replace HeatCapacity/heat capacity with SpecificHeat / C = m×c where relevant.
- **Optional**: Remove legacy constants (DEFAULT_GAS_HEAT_CAPACITY_J_PER_K etc.) once no code path uses them.

---

Energy view.

particles with drag, to lose energy to air

Particle colisions generate pressure and heat.