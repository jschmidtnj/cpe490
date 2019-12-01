<template>
  <div>
    <v-select
      v-if="organizer"
      :options="chatOptions"
      v-model="destination"
      label="label"
    />
    <Chat
      :participants="participants"
      :myself="myself"
      :messages="messages"
      :onType="onType"
      :onMessageSubmit="onMessageSubmit"
      :chatTitle="'Emergency Services'"
      :placeholder="'send message'"
      :colors="colors"
      :borderStyle="borderStyle"
      :hideCloseButton="false"
      :closeButtonIconSize="'20px'"
      :submitIconSize="'30px'"
      :asyncMode="false"
      class="chat mt-2"
    >
      <template v-slot:header>
        <div>
          <p
            v-for="(participant, index) in participants"
            :key="index"
            class="custom-title"
          >
            {{ participant.name }}
          </p>
        </div>
      </template>
    </Chat>
  </div>
</template>

<script>
import Chat from '~/components/chat/Chat.vue'
import { chatConf } from '~/assets/config'
const emergencyOptions = []

for (let i = 0; i < chatConf.emergencyServicesAddresses.length; i++) {
  emergencyOptions.push({
    label: `emergency services ${i + 1}`,
    value: chatConf.emergencyServicesAddresses[i]
  })
}

const messageType = '\x00'

export default {
  components: {
    Chat
  },
  data() {
    return {
      participants: [
        {
          name: 'Emergency Services',
          id: 2
        }
      ],
      myself: {
        name: 'me',
        id: 1
      },
      socket: null,
      organizer: false,
      source: '',
      destination: null,
      chatOptions: [],
      messages: [],
      colors: {
        header: {
          bg: '#2143cc',
          text: '#ffffff'
        },
        message: {
          myself: {
            bg: '#fff',
            text: '#0a0a0a'
          },
          others: {
            bg: '#fb4141',
            text: '#0a0a0a'
          },
          messagesDisplay: {
            bg: '#f7f3f3'
          }
        },
        submitIcon: '#b91010'
      },
      borderStyle: {
        topLeft: '10px',
        topRight: '10px',
        bottomLeft: '10px',
        bottomRight: '10px'
      }
    }
  },
  beforeDestroy() {
    if (this.socket) this.socket.close()
    this.messages = []
  },
  mounted() {
    this.$axios
      .get(chatConf.configEndpoint)
      .then((res) => {
        if (res.status === 200 && res.data.hasOwnProperty('source')) {
          if (
            emergencyOptions.find(
              (option) => option.value.charCodeAt(0) === res.data.source
            )
          ) {
            this.organizer = true
            // the current node is the emergency worker
            this.updateChatSelection()
          } else {
            // the current node is a person who needs help
            this.destination = emergencyOptions[0]
          }
          this.socket = new window.WebSocket(
            `ws://${process.env.apiurl.replace('http://', '')}${
              chatConf.websocketEndpoint
            }`
          )
          this.source = res.data.source
          this.socket.onopen = (evt) => {
            this.$toasted.global.success({
              message: 'Connection established'
            })
          }
          this.socket.onmessage = (evt) => {
            // console.log(`[message] Data received from server: ${evt.data}`)
            let jsonData
            try {
              jsonData = JSON.parse(evt.data)
            } catch (err) {
              this.$toasted.global.error({ message: JSON.stringify(err) })
            }
            if (jsonData.hasOwnProperty('debug')) {
              console.log(jsonData.debug)
            } else if (jsonData.hasOwnProperty('message')) {
              this.messages.push(jsonData.message)
            }
          }
          this.socket.onclose = (evt) => {
            if (evt.wasClean) {
              console.log(
                `[close] Connection closed cleanly, code=${evt.code} reason=${evt.reason}`
              )
            } else {
              this.$toasted.global.error({ message: '[close] Connection died' })
            }
          }
          this.socket.onerror = (err) => {
            console.error(`[error]: `, err)
          }
        } else {
          this.$toasted.global.warn({ message: 'no source found' })
        }
      })
      .catch((err) => {
        const message = 'got error with get config request'
        this.$toasted.global.error({ message })
        console.log(message, err)
      })
  },
  methods: {
    onType(event) {
      // here you can set any behavior
    },
    onMessageSubmit(message) {
      this.messages.push(message)
      console.log(`New message sending: ${JSON.stringify(message)}`)
      if (this.destination) {
        const newMessage =
          this.destination.value + messageType + message.content
        this.socket.send(newMessage)
        message.uploaded = true
      } else {
        this.$toasted.global.error({ message: 'no destination found' })
      }
    }
  }
}
</script>

<style lang="scss" scoped>
.chat {
  min-height: 40rem;
  max-width: 30rem;
}
.chat .custom-title {
  color: #ffffff;
  font-size: 20px;
}
</style>
