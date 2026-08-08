#include "GameSound.h"

namespace GameAPI {
    void GameSound::play(GameActor actor, float volume) {
        RE::BSAudioManager::GetSingleton()->GetSoundHandle(handle, form, 0x10); //for vr version
        handle.SetObjectToFollow(actor.form->Get3D());
        handle.SetVolume(volume);
        handle.Play();
    }
}