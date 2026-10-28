# Ninajirachi Wearables

Shared language for the wearable Ninajirachi experience and its interactions.

## Participants

**Wearable**:
A battery-powered Ninajirachi device worn by a person as a necklace or hairclip.
_Avoid_: unit, accessory

**Peer**:
Another Ninajirachi wearable that can participate in a nearby interaction.
_Avoid_: user, device target

## Interaction

**Approximate gaze**:
A playful left, center, or right eye direction inferred from a peer's changing signal strength while the wearer turns.
_Avoid_: tracking, localization, precise direction

**Centered-eye fallback**:
The neutral gaze shown when the wearable lacks reliable directional evidence or shortly after an approximate gaze interaction ends.
_Avoid_: lost-target state

**Nearby**:
A peer state supported by recent wireless discovery evidence, without promising a fixed physical distance.
_Avoid_: in range, close, localized

**Focused interaction**:
The interaction mode used when one nearby peer is selected as the current target.
_Avoid_: one-to-one mode

**Sustained proximity**:
Nearby status that remains active long enough to trigger an excited interaction, even when only one peer is present.
_Avoid_: group presence

**Excited interaction**:
The heightened animation reached after sustained proximity with a nearby peer.
_Avoid_: group mode, multiplayer mode

**Stale peer**:
A peer whose discovery evidence has been absent long enough that it no longer counts as nearby.
_Avoid_: disconnected user

**Transient overlay**:
A temporary visual reaction that takes priority over the current interaction while preserving that interaction underneath.
_Avoid_: queued reaction, replacement state

**Discovery**:
The wireless evidence used to identify a peer and decide whether it is still nearby.
_Avoid_: tracking, ranging

**Direction evidence**:
Compass-heading and signal-strength observations used to make an approximate gaze choice for the selected peer.
_Avoid_: position, bearing, localization

**Settings/battery screen**:
A temporary screen opened by holding the touchscreen for five seconds to inspect wearable settings and battery state.
_Avoid_: debug screen, touch mode

**Face screen**:
The normal visual screen where the Ninajirachi expression and interactions are shown.
_Avoid_: home screen, default screen

**Parameterized face**:
The runtime face rendered from expression values such as eye openness, pupil offset, mouth shape, and effects rather than a collection of expression image files.
_Avoid_: sprite face, image state

**Expression preset**:
A named set of parameter values that defines a recognizable Ninajirachi emotion, such as idle, curious, happy, surprised, or excited.
_Avoid_: animation file, face asset

**Desktop face preview**:
A desktop GUI used to tune and preview expression presets at the wearable's display size; it is not part of the wearable runtime.
_Avoid_: wearable GUI, on-device editor
