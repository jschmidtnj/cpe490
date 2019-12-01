export default {
  newMessage: (state, message) => {
    message.timestamp = new Date().getTime()
    state.messages = [...state.messages, message]
  },
  setParticipants: (state, participants) => {
    state.participants = participants
  },
  setMyself: (state, myself) => {
    state.myself = myself
  },
  setMessages: (state, messages) => {
    state.messages = messages
  },
  setChatTitle: (state, title) => {
    state.chatTitle = title
  },
  setPlaceholder: (state, placeholder) => {
    state.placeholder = placeholder
  }
}
