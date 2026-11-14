# Lobby: Heaven to Start

The round starts in `Heaven.umap` and ends with every player at Start on the round's map.

1. **Podium.** Players gather on the Heaven podium. The host starts the round.
2. **Gate opens.** The Heaven gate opens and players drop off the edge.
3. **The fall is the load screen.** While players fall, each client loads the round's map and reports loaded to the server. The fall lasts until every player has reported loaded, or 20 s have passed, whichever comes first.
4. **Server sends the seed.** The server picks one seed per round and replicates it. Every client runs `generate(map, seed)`, so all clients build the same layout.
5. **Land at Start.** Everyone lands at the map's Start (1 m into section 1). A player still loading at 20 s lands when their load finishes.

## Late joiners
A player who joins after the fall skips Heaven: they get the round's seed on join, load the map, and drop in at Start.

## Server rules
- The server owns the seed and the 20 s timer; clients never pick a seed.
- Start and the layout come from the same `generate(map, seed)` described in `maps.md`.
