# ClueSolver
Simple solver for the game of Clue, both Classic and Master Detective rules.
## Command syntax:
cluesolver [-c *file*] [-o *file*] [*file*]
### -c *file*
If this option is specified, the rules and card names are loaded from the specified file. The file should hold valid a JSON object with
the following elements:
```javascript
{
  "rules" : "master",
  "suspects" : [ "mustard", ... , "peach" ],
  "weapons" : [ "revolver", ..., "horseshoe" ],
  "rooms" : [ "dining", ..., "fountain" ]
}
```
Valid values for the "rule" element are "master" or "classic" If any elements are missing, the Classic Clue values are assumed.
### -o *file*
If this option is specified, all output goes to the named file. Otherwise, all output goes to the console.
### *file*
If specified, input comes from this file. Otherwise, input comes from the console.
## Input
Input consists of a JSON array of player names on a single line, followed JSON objects on single lines describing events in the game.
### Players
A simple JSON array of names. For example,
```javascript
["joe","chris","dave","liz"]
```
### Events
#### hand
The first event should be a **hand** event if you are observing a player. The event value is an object containing a a `player` element
and a `cards` array. For example,
```javascript
{ "hand" : { "player" : "joe", "cards" : [ "plum", ... ,"studio" ] } }
```
#### suggest
A **suggest** event describes a suggestion and the results. Note that the interpretation of the results depends on the rules. The event
value is an object containing a a `player` element describing the player making the suggestion, a `cards` array decribing the cards in
the suggestion, and a `showed` array describing the actions of the other players. For example,
```javascript
{ "suggest" : { "player" : "joe", "cards" : [ "mustard", "knife", "billiard" ], "showed" : [ "chris" , "liz" ] } }
```
##### Master Detective Rules
If Master Detective rules are being used, then any player that has a suggested card must show it. All players that show a card are
listed.
##### Classic Rules
If Classic rules are used, then only one card is shown. The players that did or did not have a card are listed, but the last player
is the player that showed a card.
##### show
One or more **show** events may occur after a **suggest** event if any cards are shown to you. The event value is an object containing
a `player` element describing the player that showed the card and a `card` element describing the card that was shown. For example,
```javascript
{ "show" : { "player" : "chris" , "card" : "billiard" } }
{ "show" : { "player" : "liz", "card" : "mustard" } }
```
